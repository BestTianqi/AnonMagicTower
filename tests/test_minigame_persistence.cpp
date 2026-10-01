#include "Game/SlidingPuzzleState.h"
#include "Game/Game2048State.h"
#include <cstdlib>
#include <iostream>
static void check(bool ok,const char* reason){if(!ok){std::cerr<<reason<<'\n';std::exit(1);}}
int main(){
    for(int n=3;n<=8;++n){
        SlidingPuzzleState original(n);original.shuffle(731);
        for(int t=0;t<30;++t)for(int i=0;i<n*n;++i)if(original.canMove(i)){original.move(i);break;}
        SlidingPuzzleState loaded;
        check(loaded.restore(original.serialize()),"puzzle restore");
        check(loaded.size()==n && loaded.tiles()==original.tiles() && loaded.moves()==original.moves(),"puzzle board and steps round trip");
        while(original.moves()){check(loaded.undo() && original.undo(),"puzzle undo restored");check(loaded.tiles()==original.tiles(),"puzzle undo order preserved");}
        const auto before=loaded.tiles();check(!loaded.restore("broken") && loaded.tiles()==before,"bad puzzle leaves state intact");
    }
    for(unsigned seed=1;seed<=12;++seed){
        Game2048State original(seed);
        for(int t=0;t<80;++t)original.move(static_cast<Game2048State::Direction>(t%4));
        Game2048State loaded;
        check(loaded.restore(original.serialize()),"2048 restore");
        check(loaded.tiles()==original.tiles() && loaded.score()==original.score() && loaded.moves()==original.moves(),"2048 round trip");
        const auto before=loaded.tiles();check(!loaded.restore("broken") && loaded.tiles()==before,"bad 2048 leaves state intact");
        for(int t=0;t<8;++t){check(loaded.undo()==original.undo(),"2048 undo availability");check(loaded.tiles()==original.tiles(),"2048 undo round trip");}
        for(int t=0;t<16;++t){auto d=static_cast<Game2048State::Direction>(t%4);check(loaded.move(d)==original.move(d),"2048 replay availability");check(loaded.tiles()==original.tiles(),"2048 random state must round trip");}
    }
    std::cout<<"mini-game persistence and undo passed\n";
}
