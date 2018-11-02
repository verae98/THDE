#ifndef _RUNGAMECONTROLLER_HPP
#define _RUNGAMECONTROLLER_HPP


#include "hwlib.hpp"
#include "rtos.hpp"
#include "buzzer.hpp"
#include "ir_send.hpp"
#include "OLEDcontroller.hpp"
#include "msg.hpp"
#include "commandListener.hpp"

class runGameController : public rtos::task<>, public commandListener{
    
private:
    buzzer & bz;
    ir_send & encoder;
	OLEDcontroller & display;
    bool values[16] = {1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0};
 
    struct playerInfo{
        uint8_t playerNR;
        uint8_t dmg;
    };
    
    int HP_total;
    int HP;
    
    rtos::channel<playerInfo, 1024> playerInfoQueue;
    rtos::channel<msg, 1024> cmdChannelIn;
    
    enum class STATE {STARTUP, RUNNING, DEAD, GAMEOVER};
	enum STATE state;
    rtos::timer timeout_timer;
    int gameoverTime = 6000000;
    int killedTime = 6000000;
    commandListener * cl;
    
    
    void main() override {
        uint8_t spelerID = 0;
        uint8_t dmg = 3;
        bool readyToStart = 0;
        while(1){
            switch(state) {
                case STATE::STARTUP:
                    {
                        msg message = cmdChannelIn.read();
                        if(message.command == message.CMD::R_PLAYER_NAME){
                            hwlib::cout<< "R_PLAYER_NAME \n";
                            display.showYourName(message.naam);
                        } else if (message.command == message.CMD::R_PLAYER_ID){
                            spelerID = message.waarde;
                            hwlib::cout<< "R_PLAYER_ID \n";
                        } else if (message.command == message.CMD::R_SELECTED_DMG){
                            dmg = message.waarde/10;
                            hwlib::cout<< dmg << "R_SELECTED_DMG \n";
                        } else if (message.command == message.CMD::R_START_GAME){
                            hwlib::cout<< "R_START_GAME \n";
                            readyToStart = 1;
                            if (spelerID == 0){
                                msg m = {};
                                m.command = msg::CMD::T_REQ_PLAYERID;
                                cl -> commandReceived(m);
                            } else {
                                encoder.setIrpattern(spelerID, dmg);
                                encoder.enable();
                                hwlib::cout << "running!!";
                                state = STATE::RUNNING;
                            }
                        } else if (message.command == message.CMD::R_HP){
                            hwlib::cout<< "R_HP \n";
                            HP_total = message.waarde;
                            HP = HP_total;
                        } else if (spelerID > 0 && readyToStart){
                            hwlib::cout<< "staaarrtt \n";
                            encoder.setIrpattern(spelerID, dmg);
                            encoder.enable();
                            state = STATE::RUNNING;
                        } 
                    }
                    
                    break;
                    
                case STATE::RUNNING:
                    {
                        auto done = wait(playerInfoQueue + cmdChannelIn);
                        if (done == playerInfoQueue){
                            auto pi = playerInfoQueue.read();
                            auto dmg_enemy = pi.dmg;
                            HP -= dmg_enemy*10;
                            
                            if (HP <= 0){
                                hwlib::cout<< "killed!";
                                msg m = {};
                                m.command = msg::CMD::T_KILLED_BY;
                                m.waarde = pi.playerNR;
                                cl -> commandReceived(m);
                                encoder.disable();
                                HP = HP_total;                               
                                timeout_timer.set( killedTime );
                                state = STATE::DEAD;
                            } else {
                                bz.hitSound();
                                display.showHPchanged(HP);
                                
                            }
                        } else if (done == cmdChannelIn){
                            auto cmd_msg = cmdChannelIn.read();
                            if (cmd_msg.command == cmd_msg.CMD::R_GAME_OVER){
                                hwlib::cout<< "R_GAME_OVER \n";
                                timeout_timer.set( gameoverTime );
                                encoder.disable();
                                display.showGameOver();
                                state = STATE::GAMEOVER;
                            } else if (cmd_msg.command == cmd_msg.CMD::R_LAST_MINUTE){
                                hwlib::cout<< "R_LAST_MINUTE \n";
                                bz.lastMinuteSound();
                                display.showOneMinute();
                            } else if (cmd_msg.command == cmd_msg.CMD::R_KILL_CONFIRM){
                                bz.youKilledSound();
                            }
                        }
                    }
                    hwlib::wait_us(100);
                    break;
                    
                case STATE::DEAD:
                    {
                        bz.gotKilledSound();
                        auto done = wait(timeout_timer + cmdChannelIn);
                        if (done == cmdChannelIn) {
                            msg cmd_msg = cmdChannelIn.read();
                            if (cmd_msg.command == cmd_msg.CMD::R_GAME_OVER){
                                hwlib::cout<< "R_GAME_OVER \n";
                                timeout_timer.set( gameoverTime );
                                encoder.disable();
                                display.showGameOver();
                                bz.gameOverSound();
                                state = STATE::GAMEOVER;
                            } else if (cmd_msg.command == cmd_msg.CMD::R_LAST_MINUTE){
                                hwlib::cout<< "R_LAST_MINUTE \n";
                                bz.lastMinuteSound();
                                display.showOneMinute();
                            } else if (cmd_msg.command == cmd_msg.CMD::R_KILLED_BY){
                                display.showKiller(cmd_msg.naam);
                            }
                        } else {
                            hwlib::cout << "alive!!";
                            encoder.enable();
                            playerInfoQueue.clear();
                            display.showHPchanged(HP);
                            state = STATE::RUNNING;
                        }
                    }
                    break;
                
                case STATE::GAMEOVER:
                    auto done = wait(timeout_timer);
                    if (done == timeout_timer){
                        state = STATE::STARTUP;
                    }
                    break;
            }
        }
    }
    
public:
    runGameController(buzzer & bz, ir_send & encoder, OLEDcontroller & display, commandListener * cl = nullptr):
    task(7, "runGameTask"),
    bz (bz),
    encoder ( encoder ),
	display(display),
    HP_total (100),
    HP (100),
    playerInfoQueue(this, "playerInfoQueue"),
    cmdChannelIn(this, "cmdChannelIn"),
    state(STATE::STARTUP),
    timeout_timer (this, "timeout_timer"),
    cl (cl)
    {}
    
    void sendPlayerInfo(uint8_t playerNR, uint8_t dmg){
        playerInfo pi{playerNR, dmg};
        playerInfoQueue.write(pi);
    }
    
    void commandReceived(const msg & m){
        cmdChannelIn.write(m);
    }
    
    void setListener(commandListener * cl_def){
        cl = cl_def;
    }

    
    
    
};


#endif