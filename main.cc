/* 
 * File:   main.cc
 * Author: Nathan Duma
 *
 * Created on April 30, 2018, 12:27 AM
 */

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
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

int main(int argc, char *argv[]) {

    cout << "mksprite64 by Nathan Duma." << endl;
    cout << "Convert png to c source file for the Nintendo 64, using the sprite/bitmap structure." << endl;
    cout << "Type -h for usage" << endl;
    if (argc == 1) {
        cerr << "ERROR 1: no argument provided." << endl;
        return 1;
    }

    string file, filename, scaleX, scaleY;

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
            } else {
                cerr << "ERROR 3: Unknown command: " << argv[i] << endl;
                return 3;
            }
        }
    }

    // decode the image
    vector<unsigned char> png;
    vector<unsigned char> image; //the raw pixels
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

    fstream f(filename + ".c");

    // header
    f << "#include <ultra64.h>" << endl;
    f << endl;

    // definitions
    f << "#define " << filename << "BLOCKSIZEW\t" << "40" << endl;
    f << "#define " << filename << "BLOCKSIZEH\t" << "40" << endl;
    f << "#define " << filename << "SCALEX\t" << scaleX << endl;
    f << "#define " << filename << "SCALEY\t" << scaleY << endl;
    f << "#define " << filename << "ALPHABIT\t" << "255" << endl;
    f << "#define " << filename << "MODE\t" << "SP_TRANSPARENT" << endl;
    f << endl;

    // preview
    if (preview) {
        f << "#if 0	/* Image preview */" << endl;
        displayPreview(image, width, height, f);
        f << "#endif" << endl;
        f << endl;
    }

    // extern functions
    f << "extern void SPDraw_" << filename <<
            "(Gfx **g, int xx, int yy, float Xscale, float Yscale, int A)" << endl;
    f << "extern void BMDraw_" << filename <<
            "(Gfx **g, int num, int xx, int yy, float Xscale, float Yscale, int A)" << endl;
    f << endl;
    f << endl;

    // dummy aligner
    f << "static Gfx " << filename
            << "_C_dummy_aligner[] = { gsSPEndDisplayList() };" << endl;
    f << endl;

    
    // TODO: output colour
    
    for (int i = 0; i < image.size(); i += 4) {
        int r = image[i] / 8;
        int g = image[i + 1] / 8;
        int b = image[i + 2] / 8;
        int a = (image[i + 3] > 0 ? 1 : 0);

        int colour = (r << 11) | (g << 6) | (b << 1) | a;
    }

    f.close();

    return 0;
}














































































































