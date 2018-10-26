#include "hwlib.hpp"
#include "rtos.hpp"
#include "ir_decoder.hpp"

class ir_detector: public rtos::task<>
{
private:
	hwlib::target::pin_in & detector;
	ir_decoder & decoder;
	uint16_t firstSet = 0;
	uint16_t secondSet = 0;
	enum class STATE {WAITING, MESSAGING};
	enum STATE state;
	int counter = 0;
	int timerValue = 100;
	

	void main( void ) override
	{
		for(;;)
		{
			switch(state)
			{
				case STATE::WAITING:
					if(decoder.get() == 0)
					{
						counter++;
						firstSet = 1;
						timerValue = 1000;
						state = STATE::MESSAGING;
					}
					break;
				
				case STATE::MESSAGING:
					if(counter / 16 == 0)
					{
						firstSet = firstSet << 1;
                        firstSet |= !decoder.get();
					}
					else
					{
						secondSet = secondSet << 1;
                        secondSet |= !decoder.get();
					}
					if (counter == 15)
					{
						hwlib::wait_us(2980);
					}
					counter++;
					
					if(counter >= 31)
					{
                        hwlib::cout<<firstSet<< " /n";
						hwlib::cout << " ================================";
                        hwlib::cout<<secondSet<< " /n";
						
                        counter = 0;
						timerValue = 100;
						state = STATE::WAITING;
					}
					break;
			}
			hwlib::wait_us(timerValue);
		}
	}


public:
ir_detector(ir_decoder & decoder, hwlib::target::pin_in & detector):
	task(1, "detector_task"),
	decoder ( decoder ),
	detector(detector),
	state(STATE::WAITING)
	{}
	


};