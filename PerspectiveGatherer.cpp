#include "PerspectiveGatherer.h"
#include "Options.h"

std::map<char, FaceDetails> getFaceDetails()
{
	std::map<char, FaceDetails> output;

	output['0'] = {FRONT, "Front", "D:\\Mark\\ProgrammingWorkspace\\brlcad\\buildapple\\src\\gtools\\rgen\\visualization\\src\\ortho_front.png", LENGTH, HEIGHT};
	output['1'] = {TOP, "Top", "D:\\Mark\\ProgrammingWorkspace\\brlcad\\buildapple\\src\\gtools\\rgen\\visualization\\src\\ortho_top.png", LENGTH, DEPTH};
	output['2'] = {RIGHT, "Right", "D:\\Mark\\ProgrammingWorkspace\\brlcad\\buildapple\\src\\gtools\\rgen\\visualization\\src\\ortho_right.png", DEPTH, HEIGHT};
	output['3'] = {LEFT, "Left", "D:\\Mark\\ProgrammingWorkspace\\brlcad\\buildapple\\src\\gtools\\rgen\\visualization\\src\\ortho_left.png", DEPTH, HEIGHT};
	output['4'] = {BACK, "Back", "D:\\Mark\\ProgrammingWorkspace\\brlcad\\buildapple\\src\\gtools\\rgen\\visualization\\src\\ortho_back.png", LENGTH, HEIGHT};
	output['5'] = {BOTTOM, "Bottom", "D:\\Mark\\ProgrammingWorkspace\\brlcad\\buildapple\\src\\gtools\\rgen\\visualization\\src\\ortho_bottom.png", LENGTH, DEPTH};
	output['6'] = {BACK_MIRRORED, "Back-Mirrored", "D:\\Mark\\ProgrammingWorkspace\\brlcad\\buildapple\\src\\gtools\\rgen\\visualization\\src\\ortho_back_mirrored.png", LENGTH, HEIGHT};
	output['7'] = {BOTTOM_MIRRORED, "Bottom-Mirrored", "D:\\Mark\\ProgrammingWorkspace\\brlcad\\buildapple\\src\\gtools\\rgen\\visualization\\src\\ortho_bottom_mirrored.png", LENGTH, DEPTH};
	output['8'] = {DETAILED, "Ambient Occlusion", "D:\\Mark\\ProgrammingWorkspace\\brlcad\\buildapple\\src\\gtools\\rgen\\visualization\\src\\ambient_occ.png"};

	return output;
}

std::string extractFileName(std::string filePath) {
    return filePath.substr(filePath.find_last_of("/\\") + 1);
}

std::string renderPerspective(RenderingFace face, Options& opt)
{
    // hardcode filename until options come out
    std::string component = "component";
    std::string pathToInput = opt.getFilepath();
    std::string fileInput = extractFileName(pathToInput);
    std::string pathToOutput = "output/";
    std::string fileOutput = fileInput.substr(0, fileInput.find_last_of("."));
    std::cout << "dbg " << pathToInput << " " << fileInput << " " << fileOutput << std::endl;

    // do directory traversal checks
    if (fileOutput.find("../") != std::string::npos) {
        std::cout << "ERROR: Output file name cannot contain ../\n";
        return "";
    }

    std::cout << "Processing file: " << fileInput << std::endl;

    // FIX security vulnerability
    std::string outputname = pathToOutput + fileOutput;
    std::string render;

    int a, e;
    switch (face) {
        case FRONT:
            a = 0, e = 0;
            outputname += "_front.png";
            render = "../../../build/bin/rtedge -s 1024 -W -R -a " + std::to_string(a) + " -e " + std::to_string(e) + " -o " + outputname + " " + pathToInput + " " + component;
            break;
        case RIGHT:
            a = 90, e = 0;
            outputname += "_right.png";
            render = "../../../build/bin/rtedge -s 1024 -W -R -a " + std::to_string(a) + " -e " + std::to_string(e) + " -o " + outputname + " " + pathToInput + " " + component;
            break;
        case BACK:
            a = 180, e = 0;
            outputname += "_back.png";
            render = "../../../build/bin/rtedge -s 1024 -W -R -a " + std::to_string(a) + " -e " + std::to_string(e) + " -o " + outputname + " " + pathToInput + " " + component;
            break;
        case LEFT:
            a = 270, e = 0;
            outputname += "_left.png";
            render = "../../../build/bin/rtedge -s 1024 -W -R -a " + std::to_string(a) + " -e " + std::to_string(e) + " -o " + outputname + " " + pathToInput + " " + component;
            break;
        case TOP:
            a = 0, e = 90; // may need to change "a"?
            outputname += "_top.png";
            render = "../../../build/bin/rtedge -s 1024 -W -R -a " + std::to_string(a) + " -e " + std::to_string(e) + " -o " + outputname + " " + pathToInput + " " + component;
            break;
        case BOTTOM:
            a = 0, e = 270;
            outputname += "_bottom.png";
            render = "../../../build/bin/rtedge -s 1024 -W -R -a " + std::to_string(a) + " -e " + std::to_string(e) + " -o " + outputname + " " + pathToInput + " " + component;
            break;
        case DETAILED:
            a = 45, e = 45;
            outputname += "_detailed.png";
            render = "../../../build/bin/rt -C 255/255/255 -s 1024 -A 1.5 -W -R -c \"set ambSamples=64\" -o " + outputname + " " + pathToInput + " " + component;
            break;
        default:
            std::cerr<< "mark added this\n";
            break;
    }

    std::ifstream file;
    file.open(outputname);
    if (file) {
        std::string rmFile = "rm " + outputname;
        auto result2 = system(rmFile.c_str());
    }
    file.close();
    
    try {
        auto result2 = system(render.c_str());
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    std::cout << "Successlly generated perspective rendering png file\n";
	return outputname;
}