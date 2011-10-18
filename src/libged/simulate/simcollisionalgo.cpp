/*              S I M C O L L I S I O N A L G O . C P P
 * BRL-CAD
 *
 * Copyright (c) 2011 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/*
 * Routines related to performing collision detection using rt.  This
 * is a custom algorithm that replaces the box-box collision algorithm
 * in Bullet.
 */

#include "common.h"

#ifdef HAVE_BULLET

/* system headers */
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btBoxBoxDetector.h>

/* private headers */
#include "./simcollisionalgo.h"


#define USE_PERSISTENT_CONTACTS 1

//#define DEBUG_MF 1


btRTCollisionAlgorithm::btRTCollisionAlgorithm(btPersistentManifold* mf,
					       const btCollisionAlgorithmConstructionInfo& ci,
					       btCollisionObject* obj0,
					       btCollisionObject* obj1)
    : btActivatingCollisionAlgorithm(ci, obj0, obj1),
      m_ownManifold(false),
      m_manifoldPtr(mf)
{
    if (!m_manifoldPtr && m_dispatcher->needsCollision(obj0, obj1)) {
	m_manifoldPtr = m_dispatcher->getNewManifold(obj0, obj1);
	m_ownManifold = true;
    }
}


btRTCollisionAlgorithm::~btRTCollisionAlgorithm()
{
    if (m_ownManifold) {
	if (m_manifoldPtr)
	    m_dispatcher->releaseManifold(m_manifoldPtr);
    }
}


void
btRTCollisionAlgorithm::processCollision(btCollisionObject* body0,
					 btCollisionObject* body1,
					 const btDispatcherInfo& dispatchInfo,
					 btManifoldResult* resultOut)
{
    if (!m_manifoldPtr)
	return;

    btCollisionObject* col0 = body0;
    btCollisionObject* col1 = body1;
#ifndef DEBUG_MF
    btBoxShape* box0 = (btBoxShape*)col0->getCollisionShape();
    btBoxShape* box1 = (btBoxShape*)col1->getCollisionShape();
#endif //DEBUG_MF

    //quellage
    bu_log("%d", dispatchInfo.m_stepCount);

    /// report a contact. internally this will be kept persistent, and contact reduction is done
    resultOut->setPersistentManifold(m_manifoldPtr);
#ifndef USE_PERSISTENT_CONTACTS
    m_manifoldPtr->clearManifold();
#endif //USE_PERSISTENT_CONTACTS

    btDiscreteCollisionDetectorInterface::ClosestPointInput input;
    input.m_maximumDistanceSquared = BT_LARGE_FLOAT;
    input.m_transformA = body0->getWorldTransform();
    input.m_transformB = body1->getWorldTransform();

#ifndef DEBUG_MF
    //This part will get replaced with a call to rt
    btBoxBoxDetector detector(box0, box1);
    detector.getClosestPoints(input, *resultOut, dispatchInfo.m_debugDraw);
#endif //DEBUG_MF

    //------------------- DEBUG ---------------------------

    int i, j;

    //Get the user pointers to struct rigid_body, for printing the body name
    struct rigid_body *rbA = (struct rigid_body *)col0->getUserPointer();
    struct rigid_body *rbB = (struct rigid_body *)col1->getUserPointer();

    if (rbA != NULL && rbB != NULL) {


    	//Iterate over the points for the BT manifold----------------------------------

    	btPersistentManifold* contactManifold = resultOut->getPersistentManifold();

		if(rbB->num_bt_manifolds < MAX_MANIFOLDS){
			i = rbB->num_bt_manifolds;

			/* Create a new manifold IN BODY B ONLY */
			rbB->bt_manifold[i].rbA = rbA;
			rbB->bt_manifold[i].rbB = rbB;

			bu_log("processCollision(box/box): Overlap between %s & %s \n", rbA->rb_namep, rbB->rb_namep);

			//Get the number of points in this manifold
			rbB->bt_manifold[i].num_contacts = contactManifold->getNumContacts();

			bu_log("processCollision : Manifold contacts : %d\n", rbB->bt_manifold[i].num_contacts);


			for (j=0; j<rbB->bt_manifold[i].num_contacts; j++) {
				btManifoldPoint& pt = contactManifold->getContactPoint(j);

				btVector3 ptA = pt.getPositionWorldOnA();
				btVector3 ptB = pt.getPositionWorldOnB();

				VMOVE(rbB->bt_manifold[i].contacts[j].ptA, ptA);
				VMOVE(rbB->bt_manifold[i].contacts[j].ptB, ptB);
				VMOVE(rbB->bt_manifold[i].contacts[j].normalWorldOnB, pt.m_normalWorldOnB);

				bu_log("processCollision: Got BT contact %d, %s(%f, %f, %f) , \
						%s(%f, %f, %f), n(%f, %f, %f)\n",
				   j+1,
				   rbA->rb_namep, ptA[0], ptA[1], ptA[2],
				   rbB->rb_namep, ptB[0], ptB[1], ptB[2],
				   pt.m_normalWorldOnB[0], pt.m_normalWorldOnB[1], pt.m_normalWorldOnB[2]);

			}

			rbB->num_bt_manifolds++;
		}
		else{
			bu_log("processCollision(box/box): %s has got %d manifold which is the maximum allowed\n",
					rbB->rb_namep, rbB->num_bt_manifolds);
		} //end-if(rbB->num_bt_manifolds...


	    //Scan all RT manifolds of rbB looking for a rbA-rbB manifold----------------------
#ifdef DEBUG_MF
		struct sim_manifold *rt_mf = &(rbB->rt_manifold);

		bu_log("processCollision : Scanning rbB = %s, comparing \
				%s & %s  - %s & %s\n",
				rbB->rb_namep,
				rt_mf->rbA->rb_namep,
				rbA->rb_namep,
				rt_mf->rbB->rb_namep,
				rbB->rb_namep);

		//Find the manifold in rbB's list which connects rbB and rbA
		if( BU_STR_EQUAL(rt_mf->rbA->rb_namep, rbA->rb_namep) &&
				BU_STR_EQUAL(rt_mf->rbB->rb_namep, rbB->rb_namep) ){

		   // Now add the RT contact pairs
		   for (i=0; i<rt_mf->num_contacts; i++){

			   btVector3 ptA, ptB, normalWorldOnB;

			   VMOVE(ptA, rt_mf->contacts[i].ptA);
			   VMOVE(ptB, rt_mf->contacts[i].ptB);
			   VMOVE(normalWorldOnB, rt_mf->contacts[i].normalWorldOnB);

			   //Positive depth for penetration
			   resultOut->addContactPoint(normalWorldOnB, ptB, rt_mf->contacts[i].depth);

			   bu_log("processCollision: Added RT contact %d, %s(%f, %f, %f) , \
					   %s(%f, %f, %f), n(%f, %f, %f), %f\n",
						   i+1,
						rt_mf->rbA->rb_namep, V3ARGS(ptA),
						rt_mf->rbB->rb_namep, V3ARGS(ptB),
						V3ARGS(normalWorldOnB),
						rt_mf->contacts[i].depth);
		   }

	   }//end- if( bu_strcmp...


#endif //DEBUG_MF

    } //end- if (rbA != NULL && rbB...





    //------------------------------------------------------

#ifdef USE_PERSISTENT_CONTACTS
    // refreshContactPoints is only necessary when using persistent contact points.
    // otherwise all points are newly added
    if (m_ownManifold) {
	resultOut->refreshContactPoints();
    }
#endif //USE_PERSISTENT_CONTACTS

}


btScalar
btRTCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* /*body0*/, btCollisionObject* /*body1*/, const btDispatcherInfo& /*dispatchInfo*/, btManifoldResult* /*resultOut*/)
{
    //not yet
    return 1.f;
}


#endif

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
