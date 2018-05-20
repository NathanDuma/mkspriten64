/* 
 * File:   main.cc
 * Author: Nathan Duma
 *
 * Created on April 30, 2018, 12:27 AM
 */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <math.h>   
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <ctime>
#include "lodepng.h"


using namespace std;

/*
Show ASCII art preview of the image
 */
void displayPreview(const vector<unsigned char> &image,
        unsigned w, unsigned h, fstream &f) {
    if (w > 0 && h > 0) {
        unsigned w2 = 48;
        if (w < w2) w2 = w;
        unsigned h2 = h * w2 / w;
        h2 = (h2 * 2) / 3; //compensate for non-square characters in terminal
        if (h2 > (w2 * 2)) h2 = w2 * 2; //avoid too large output

        f << '+';
        for (unsigned x = 0; x < w2; x++) f << '-';
        f << '+' << endl;
        for (unsigned y = 0; y < h2; y++) {
            f << "|";
            for (unsigned x = 0; x < w2; x++) {
                unsigned x2 = x * w / w2;
                unsigned y2 = y * h / h2;
                int r = image[y2 * w * 4 + x2 * 4 + 0];
                int g = image[y2 * w * 4 + x2 * 4 + 1];
                int b = image[y2 * w * 4 + x2 * 4 + 2];
                int a = image[y2 * w * 4 + x2 * 4 + 3];
                int lightness = ((r + g + b) / 3) * a / 255;
                int min = (r < g && r < b) ? r : (g < b ? g : b);
                int max = (r > g && r > b) ? r : (g > b ? g : b);
                int saturation = max - min;
                int letter = 'i'; //i for grey, or r,y,g,c,b,m for colors
                if (saturation > 32) {
                    int h = lightness >= (min + max) / 2;
                    if (h) letter = (min == r ? 'c' : (min == g ? 'm' : 'y'));
                    else letter = (max == r ? 'r' : (max == g ? 'g' : 'b'));
                }
                int symbol = ' ';
                if (lightness > 224) symbol = '@';
                else if (lightness > 128) symbol = letter - 32;
                else if (lightness > 32) symbol = letter;
                else if (lightness > 16) symbol = '.';
                f << (char) symbol;
            }
            f << "|";
            f << endl;
        }
        f << '+';
        for (unsigned x = 0; x < w2; x++) f << '-';
        f << '+' << endl;
    }
}

template<typename T>
string T_to_hex(T i) {
    stringstream stream;
    stream << "0x"
            << setfill('0') << setw(sizeof (T)*2)
            << hex << i;
    return stream.str();
}

int main(int argc, char *argv[]) {

    cout << "mksprite64 by Nathan Duma." << endl;
    cout << "Convert png to c source file for the Nintendo 64, using the sprite & bitmap structure." << endl;
    cout << "Type -h for usage" << endl;
    if (argc == 1) {
        cerr << "ERROR 1: no argument provided." << endl;
        return 1;
    }

    string file, filename, scaleX, scaleY, mode;

    scaleX = "1.0";
    scaleY = "1.0";
    mode = "16";

    bool preview = false;


    // parse arguments
    for (int i = 1; i < argc; i++) {
        if ((i + 1) >= argc) {
            cerr << "ERROR 2: argument-parameter mismatch" << endl;
            return 2;
        }

        if ((argv[i][0] == '-') && (argv[i][1] != '\0')) {
            if (argv[i][1] == 'h') {
                cout << "Usage:" << endl;
                cout << "Specify file path (must end in .png): -f file.png" << endl;
                cout << "OPTIONAL:" << endl;
                cout << "Scale in x direction: -sx scaleX" << endl;
                cout << "Scale in y direction: -sy scaleY" << endl;
                cout << "Scale is 1.0 by default." << endl;
                cout << "n-bit mode (n=16 or n=32): -m n" << endl;
                cout << "Mode is 16 by default." << endl;
                cout << "Show preview in c file: -p t/f" << endl;
                cout << "Preview is false by default." << endl;
            } else if (argv[i][1] == 's' && (argv[i][2] != '\0')) {
                if (argv[i][2] == 'x') {
                    scaleX = argv[i + 1];
                } else if (argv[i][2] == 'y') {
                    scaleY = argv[i + 1];
                } else {
                    cerr << "ERROR 3: Unknown command: " << argv[i] << endl;
                    return 3;
                }

                i++;
            } else if (argv[i][1] == 'f') {
                file = argv[i + 1];
                // get the filename if it's a directory or not
                size_t slashLocation = file.find_last_of("/\\");
                size_t dotLocation = file.find_last_of(".");

                filename = file.substr(
                        slashLocation == string::npos ? 0 : slashLocation + 1,
                        dotLocation == string::npos ? file.size() : dotLocation);

                i++;
            } else if (argv[i][1] == 'p') {
                if (argv[i + 1][0] == 't') {
                    preview = true;
                } else if (argv[i + 1][0] == 'f') {
                    preview = false;
                } else {
                    cerr << "ERROR 3: Unknown command: " << argv[i + 1] << endl;
                    return 3;
                }
                i++;
            } else if (argv[i][1] == 'm') {
                mode = argv[i + 1];
                // TODO: check for errors on this
                if (!(mode == "16" || mode == "32")) {
                    cerr << "ERROR 3: Unknown command: " << argv[i + 1] << endl;
                    return 3;
                }
                i++;
            } else {
                cerr << "ERROR 3: Unknown command: " << argv[i] << endl;
                return 3;
            }
        }
    }

    // decode the image
    vector<unsigned char> png;
    vector<unsigned char> image; //the raw pixels
    vector<vector < string>> fullImage;

    unsigned width, height;


    //load and decode
    unsigned error = lodepng::load_file(png, file);
    if (!error) {
        error = lodepng::decode(image, width, height, png);
    }

    //if there's an error, display it
    if (error) {
        cout << "decoder error " << error << ": " << lodepng_error_text(error) << endl;
        return error;
    }


    fullImage.resize(height);

    for (auto &it : fullImage) {
        it.resize(width);
    }

    unsigned short colour_s;
    unsigned int colour_i;

    int row = 0;
    int col = 0;

    // we need to put the image into a 2D array
    // so that we can make small sprites
    // for the bitmap structure    
    for (int i = 0; i < image.size(); i += 4) {
        if (mode == "16") {
            int r = image[i] / 8;
            int g = image[i + 1] / 8;
            int b = image[i + 2] / 8;
            int a = (image[i + 3] > 0 ? 1 : 0);

            colour_s = (r << 11) | (g << 6) | (b << 1) | a;

            fullImage[row][col] = T_to_hex(colour_s);
        } else {
            int r = image[i];
            int g = image[i + 1];
            int b = image[i + 2];
            int a = image[i + 3];

            colour_i = (r << 24) | (g << 16) | (b << 8) | a;
            fullImage[row][col] = T_to_hex(colour_i);
        }
        col++;

        if (col == width) {
            row++;
            col = 0;
        }

    }




    fstream f2("sp_" + filename + ".h", fstream::out);
    fstream f("sp_" + filename + ".c", fstream::out);


    // have all of them included in 1 h file
    // this isn't thread safe so you gotta get lucky
    // otherwise you can sleep 
    ofstream common("common_sprites.h", std::ios_base::app);

    common << "#include \"" << "sp_" << filename << ".h\"" << endl;

    common.close();

    f << "#include \"" << "sp_" + filename + ".h\"" << endl;
    f << endl;

    // header
    f2 << "#ifndef " << "sp_" << filename << "_h" << endl;
    f2 << "#define " << "sp_" << filename << "_h" << endl;
    f2 << endl;
    f2 << "#include <PR/sp.h>" << endl;

    f2 << endl;



    // split it into texels
    // use 32x32
    int texelH = 32;
    int texelW = 32;

    int splitWidth = ceil((double) width / (double) texelW);
    int splitHeight = ceil((double) height / (double) texelH);

    int totalBoxes = splitWidth*splitHeight;


    int boxX = 0;
    int boxY = 0;

    f2 << "#define " << filename << "TRUEIMAGEH\t" << height << endl;
    f2 << "#define " << filename << "TRUEIMAGEW\t" << width << endl;
    f2 << "#define " << filename << "IMAGEH\t" << texelH * splitHeight << endl;
    f2 << "#define " << filename << "IMAGEW\t" << texelW * splitWidth << endl;
    f2 << "#define " << filename << "BLOCKSIZEW\t" << texelW << endl;
    f2 << "#define " << filename << "BLOCKSIZEH\t" << texelH << endl;
    f2 << "#define " << filename << "SCALEX\t" << scaleX << endl;
    f2 << "#define " << filename << "SCALEY\t" << scaleY << endl;
    //f << "#define " << filename << "ALPHABIT\t" << "255" << endl;
    f2 << "#define " << filename << "MODE\t" << "SP_Z | SP_OVERLAP | SP_TRANSPARENT" << endl;
    f2 << endl;


    f2 << "// extern varaibles " << endl;
    f2 << "extern Bitmap " << filename << "_bitmaps[];" << endl;
    f2 << "extern Gfx " << filename << "_dl[];" << endl;
    f2 << endl;
    f2 << "#define NUM_" << filename << "_BMS  (sizeof(" << filename << "_bitmaps" << ")/sizeof(Bitmap))" << endl;
    f2 << endl;
    f2 << "extern Sprite " << filename << "_sprite;" << endl;
    f2 << endl;

    f2 << "#endif " << endl;
    f2 << endl;

    // preview output
    if (preview) {
        f2 << "#if 0	/* Image preview */" << endl;
        displayPreview(image, width, height, f2);
        f2 << "#endif" << endl;
        f2 << endl;
    }

    for (int i = 0; i < totalBoxes; i++) {
        //cout << "boxes: " << i << endl;

        vector<vector < string>> texel;

        texel.resize(texelW);

        for (auto &it : texel) {
            it.resize(texelH);
        }

        row = 0;
        col = 0;

        // now get the small texel form the big image
        for (int i = boxX * texelW; i < ((boxX * texelW) + texelW); i++) {
            for (int j = boxY * texelH; j < ((boxY * texelH) + texelH); j++) {
                if (i >= fullImage.size() ||
                        j >= fullImage[0].size()) {
                    //cout << "true" << endl;
                    texel[row][col] = "0xfffe";
                } else {
                    texel[row][col] = fullImage[i][j];
                }
                //cout << "row: " << row << " col: " << col << endl;
                col++;
            }
            row++;
            col = 0;
        }


        // dummy aligner
        f << "static Gfx " << filename << i
                << "_C_dummy_aligner[] = { gsSPEndDisplayList() };" << endl;
        f << endl;

        f << "u" << mode << " " << filename << i << "_sp" << "[] = {" << endl;

        // output stuff here
        for (auto &it : texel) {
            f << "\t";
            for (auto &it2 : it) {
                f << it2 << ", ";
            }
            f << endl;
        }


        f << endl;
        f << "};" << endl;

        f << endl;
        f << endl;

        boxY++;

        if (boxY == splitWidth) {
            boxX++;
            boxY = 0;
        }
    }

    f << endl << endl;


    f << "Bitmap " << filename << "_bitmaps[] = {" << endl;
    for (int i = 0; i < totalBoxes; i++) {
        f << "\t";
        f << "{" << filename << "BLOCKSIZEW" << ", " 
                << filename << "BLOCKSIZEW" << ", 0, 0, " 
                << filename << i << "_sp, " 
                << filename << "BLOCKSIZEH" << ", 0},";
        f << endl;
    }

    f << "};" << endl;
    f << endl;

    f << "Gfx " << filename << "_dl[NUM_DL(NUM_" << filename << "_BMS)];" << endl;
    f << endl;

    f << "Sprite " << filename << "_sprite = {" << endl;
    f << "\t" << "0, 0, /* Position: x,y */" << endl;
    f << "\t" << filename << "IMAGEW" << ", " << filename << "IMAGEH" << ", /* Sprite size in texels (x,y) */" << endl;
    f << "\t" << filename << "SCALEX" << ", " << filename << "SCALEY" << ", /* Sprite Scale: x,y */" << endl;
    f << "\t" << "0, 0, /* Sprite Explosion Spacing: x,y */" << endl;
    f << "\t" << filename << "MODE" << ", /* Sprite Attributes */" << endl;
    f << "\t" << "0x1234, /* Sprite Depth: Z */" << endl;
    f << "\t" << "255, 255, 255, 255, /* Sprite Coloration: RGBA */" << endl;
    f << "\t" << "0, 0, NULL, /* Color LookUp Table: start_index, length, address */" << endl;
    f << "\t" << "0, 1, /* Sprite Bitmap index: start index, step increment */" << endl;
    f << "\t" << "NUM_" << filename << "_BMS, /* Number of bitmaps */" << endl;
    f << "\t" << "NUM_DL(" << "NUM_" << filename << "_BMS), /* Number of display list locations allocated */" << endl;
    f << "\t" << filename << "BLOCKSIZEH" << ", " << filename << "BLOCKSIZEH" << ", /* Sprite Bitmap Height: Used_height, physical height */" << endl;
    f << "\t" << "G_IM_FMT_RGBA, /* Sprite Bitmap Format */" << endl;
    f << "\t" << "G_IM_SIZ_" << mode << "b, /* Sprite Bitmap Texel Size */" << endl;
    f << "\t" << filename << "_bitmaps, /* Pointer to bitmaps */" << endl;
    f << "\t" << filename << "_dl, /* Display list memory */" << endl;
    f << "\t" << "NULL, /* next_dl pointer */" << endl;
    f << "};" << endl;

    f << endl;


    f.close();

    return 0;
}























































































































































































































