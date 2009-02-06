puts "*** Testing 'mvall' command ***"

if {![info exists make_primitives_list]} {  
   source regression_resources.tcl
}

in_sph mvall 1
in_sph mvall 2
comb mvall_comb_1.c u mvall_sph2.s
comb moved_all_comb_2.c u mvall_sph2.s
mvall mvall_sph1.s moved_all_sph.s
mvall mvall_comb_1.c moved_all_comb1.c
mvall mvall_sph2.s moved_all_comb_sph.s

puts "*** 'mvall' testing completed ***\n"
