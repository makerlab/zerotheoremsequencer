
#include "ZeroTheorem.h"
#include "Region.h"
#include "script.h"
#include "Deck.h"

#include "boost/bind.hpp"

struct BoxArea {
    int x,y,w,h,c;
    string target;
};

static vector<string> script;
static vector<BoxArea> onevents;
static string ondone;
static int script_pc = 0;
static Region* focus;

void script_load () {
    
    script.clear();

    fs::path path = "/zerotheoremshared/script.txt"; //getOpenFilePath();
	if( path.empty() ) return;

    string line;
    char buffer[1000];
    std::cout << "Current directory is: " << getcwd(buffer, 1000) << "\n";
    ifstream myfile(path.string().c_str());
    if (myfile.is_open()) {
        while (myfile.good()) {
            getline (myfile,line);
            script.push_back(line);
        }
        myfile.close();
    } else {
        cout << "Unable to open file";
    }
    script_pc = 0;
}

void script_step() {
    if(script_pc < script.size()) script_pc++;
}

void script_set_movie(const char* filename) {
    Region* r = 0;
    for(int i = 0; i < regions.size() ; i++) {
        if(regions[i]->filename == filename) {
            r = regions[i];
            break;
        }
    }
    if(!r) {
        // we have to make the movie from scratch!
        r = new Region( 0,HEIGHT,   0,   0,   0,REGION_MOVIE,filename);
        regions.push_back(r);
    }
    // we found we already had this movie loaded earlier... just restart it
    focus = r;
}

void script_run_till_done() {

    while(1) {

        // read next command
        string command = script[script_pc];
        
        // split it
        vector <string> scratch = cinder::split( command, ' ' );
        vector <string> fields;
        for(int z = 0; z < scratch.size(); z++) {
            if(scratch[z].size() > 0) fields.push_back(scratch[z]); // remove the spaces
        }

        if(fields.size() < 1) {
            script_step();
            continue;
        }

        console() << "running " << command << endl;
        
        string term = fields[0];

        if(term == "video" && fields.size() > 1) {
            Region* r = 0;
            string filename = fields[1];
            for(int i = 0; i < regions.size() ; i++) {
                if(regions[i]->filename == filename) {
                    r = regions[i];
                    break;
                }
            }
            if(!r) {
                r = new Region(  400, 400, DECKWIDTH/DECKRATIO, DECKHEIGHT/DECKRATIO,   0,REGION_DECKV,"");
                regions.push_back(r);
                r->filename = filename;
            }
            focus = r;
        }
        
        if(term == "image" && fields.size() > 1) {
            Region* r = 0;
            string filename = fields[1];
            for(int i = 0; i < regions.size() ; i++) {
                if(regions[i]->filename == filename) {
                    r = regions[i];
                    break;
                }
            }
            if(!r) {
                r = new Region(   0,   0,1920,1080,   0,REGION_IMAGE,filename);
                regions.push_back(r);
                r->filename = filename;
            }
            focus = r;
            r->visible = 1;
        }

        else if(term == "reset" && fields.size() > 1) {
            script_set_movie(fields[1].c_str());
            if(focus) focus->reset();
        }

        else if(term == "speed" && fields.size() > 1) {
            float rate = atof(fields[1].c_str());
            if(focus) focus->rate = rate;
            console() << "set " << focus->filename << " to speed " << rate << endl;
        }

        else if(term == "play" && fields.size() > 1) {
            // find or load movie and start play from start
            script_set_movie(fields[1].c_str());
            if(focus) focus->play();
        }

        else if(term == "move" && focus && fields.size() > 2) {
            // find or load movie and start play from start
            focus->x = atoi(fields[1].c_str());
            focus->y = atoi(fields[2].c_str());
        }

        else if(term == "size" && focus && fields.size() > 2) {
            // find or load movie and start play from start
            focus->w = atoi(fields[1].c_str());
            focus->h = atoi(fields[2].c_str());
        }

        else if(term == "rotate" && focus && fields.size() > 1) {
            // find or load movie and start play from start
            focus->rotate = atoi(fields[1].c_str());
        }

        else if(term == "load" && fields.size() > 1) {
            script_set_movie(fields[1].c_str());
        }

        else if((term == "stop" || term == "pause") && fields.size() > 1) {
            script_set_movie(fields[1].c_str());
            if(focus) focus->stop();
        }

        else if(term == "hide" && fields.size() > 1) {
            script_set_movie(fields[1].c_str());
            if(focus) focus->hide();
        }

        else if(term == "show" && fields.size() > 1) {
            script_set_movie(fields[1].c_str());
            if(focus) focus->visible = 1;
            if(focus) focus->dirty = 1; // mark as dirty so that we force up a frame
            if(focus && focus->movie) focus->movie.seekToFrame( focus->cframe ); // force seek to that frame too
        }

        else if(term == "range" && fields.size() > 2 && focus) {
            int low = atoi(fields[1].c_str());
            int high = atoi(fields[2].c_str());
            if(low<0)low=0;
            if(high>focus->nframes) high=focus->nframes;
            focus->range_low = low;
            focus->range_high = high;
            focus->done = 0;
            
        }

        else if(term == "onmouse" && fields.size() > 5) {
            BoxArea box;
            box.x = atoi(fields[1].c_str());
            box.y = atoi(fields[2].c_str());
            box.w = atoi(fields[3].c_str());
            box.h = atoi(fields[4].c_str());
            box.c = 0;
            box.target = fields[5];
            onevents.push_back(box);
        }

        else if(term == "onkey" && fields.size() > 2) {
            BoxArea box;
            box.x = box.y = box.w = box.h = 0;
            box.c = atoi(fields[1].c_str());
            box.target = fields[2];
            onevents.push_back(box);
        }

        else if(term == "ondone" && fields.size() > 1) {
            ondone = fields[1];
        }
        
        else if(term == "loops" && fields.size() > 1) {
            int loopcount = atoi(fields[1].c_str());
            if(focus) {
                focus->loopcount = loopcount;
            }
        }

        else if(term == "looping" && fields.size() > 1) {
            int looping = atoi(fields[1].c_str());
            if(focus) {
                focus->looping = looping;
            }
        }

        else if(term == "target" && fields.size() > 6 && focus) {
            focus->targetx = atoi(fields[1].c_str());
            focus->targety = atoi(fields[2].c_str());
            focus->targetw = atoi(fields[3].c_str());
            focus->targeth = atoi(fields[4].c_str());
            focus->targetease = atoi(fields[5].c_str());
            focus->targetspeed = atof(fields[6].c_str());
        }

        
        // pingpong
        // animated effects on regions
        
        else if(term == "return") {
            break;
        }

        script_step();
    }
}
void script_goto(string label) {
    if(focus)focus->done = 0;
    ondone = string("");
    onevents.clear();
    for(int i = 0; i < script.size(); i++) {
        if(script[i] == label ) {
            script_pc = i;
            script_run_till_done();
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void Script::update() {
    // if we finished a movie then goto ondone event
    if(focus && focus->done && ondone.size()>0) {
        script_goto(ondone);
    }
}

void Script::setup() {
    script_load();
    script_run_till_done();

   // regions.push_back(new Region(  100, 200, 200, 75,   0,REGION_FPS,""));

}

void Script::mouseDown(MouseEvent event) {
    int x = event.getX();
    int y = event.getY();
    for(int i = 0; i < onevents.size();i++) {
        BoxArea box = onevents[i];
        if(box.x < x && x < box.x+box.w && box.y < y && y < box.y+box.h && box.target.size() > 0) {
            script_goto(box.target);
            break;
        }
    }
}

void Script::keyDown( KeyEvent event ) {

    // reset by resetting stopping and hiding all regions and then reload script from disk
    if(event.getChar() == 'r') {
        for(int i = 0; i < regions.size();i++) {
            Region* r = regions[i];
            r->reset();
            r->stop();
            r->hide();
        }
        ondone = string("");
        onevents.clear();
        focus = 0;
        script_load();
        script_run_till_done();
        return;
    }

    
    for(int i = 0; i < onevents.size();i++) {
        BoxArea box = onevents[i];
        if(box.c == event.getChar()) {
            script_goto(box.target);
            break;
        }
    }

}

// todo
//
// - have script auto detect changes
// - load other types
// - focus


