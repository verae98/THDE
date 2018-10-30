#ifndef _RUNGAMECONTROLLER_HPP
#define _RUNGAMECONTROLLER_HPP


#include "hwlib.hpp"
#include "rtos.hpp"
#include "buzzer.hpp"
#include "ir_send.hpp"
#include "OLEDcontroller.hpp"


class runGameController : public rtos::task<>{
    
private:
    buzzer & bz;
    ir_send & encoder;
	OLEDcontroller & window;
 
 
    struct playerInfo{
        int playerNR;
        int gunNR;
    };
    
    rtos::channel<playerInfo, 1024> playerInfoQueue;
    
    void main() override {
        const char* c = "hello"; 
		while(1){
            window.showHPchanged(60);
			hwlib::wait_ms(11111);
			window.showKiller(c);
			hwlib::wait_ms(11111);
			window.showOneMinute();
			hwlib::wait_ms(11111);
        }
    }
    
public:
    runGameController(buzzer & bz, ir_send & encoder, OLEDcontroller & window):
    task(7, "runGameTask"),
    bz (bz),
    encoder ( encoder ),
	window(window),
    playerInfoQueue(this, "playerInfoQueue")
    {}
    
    void sendPlayerInfo(int playerNR, int gunNR){
        playerInfo pi{playerNR, gunNR};
        playerInfoQueue.write(pi);
    }

    
    
    
};


#endif