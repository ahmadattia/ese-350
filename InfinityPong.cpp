#include "mbed.h"
#include "ShiftReg.h"
#include <math.h>
#include <MRF24J40.h>


ShiftReg::ShiftReg
(PinName data
 ,PinName store
 ,PinName clock
): _ds(data), _st(store), _sh(clock)
{
}

Ticker tick;
Serial pc(USBTX, USBRX);
InterruptIn in(p8);
InterruptIn buttonUp(p6);
InterruptIn buttonDown(p7);
Timer t1;
Timer t2;
Timer t3;
Timer t4; //this is the timer for the wireless communication
MRF24J40 mrf(p11, p12, p13, p14, p21);
char txBuffer[128];
char rxBuffer[128];
int rxLen;

int t_period;                   // This is the period between interrupts in microseconds
int interrupts;
float t_freq = 0;
int counter = 39;

const int numFrames = 40;

void drawImages();
void updateImage();
ShiftReg   HC595(p26, p25, p24);
void flip();
void shift2Bytes(int16_t bytes);
int speed;
int yVal;

int16_t square[numFrames] = {0x7E0, 0x420, 0x420, 0x420, 0x420, 0x420, 0x420, 0x7E0, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                            };
int16_t ESE350[numFrames] = {0x4002, 0x4002, 0x7FFE, 0x4002, 0x4002, 0x00, 0x00, 0xC00, 0x1E00, 0x3F80,
                             0x1FC0, 0xFE0, 0x1FC0, 0x3F80, 0x1E00, 0xC00, 0x00, 0x00, 0x7FFE, 0x7FFE,
                             0x6186, 0x6186, 0x00, 0x00, 0x7F86, 0x7F86, 0x6186, 0x6186, 0x61FE, 0x61FE,
                             0x00, 0x00, 0x7FFE, 0x7FFE, 0x6186, 0x6186, 0x00, 0x00, 0x00, 0x00
                            };
int16_t bar[numFrames] = {0xFFFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                         };
int16_t volatile image[numFrames] = {0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001,
                                     0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001,
                                     0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001,
                                     0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001
                                    };


class Paddle
{
public:
    void set_value (int);
    void draw();
};

class Ball
{
    int xPos, yPos, xSpeed, ySpeed;
public:
    void set_values (int,int,int,int);
    void draw();
};

void moveUp()
{
    if (yVal >= 11) yVal = 11;
    else yVal ++;
}

void moveDown()
{
    if (yVal <= 1) yVal = 1;
    else yVal--;
}

void Paddle::set_value(int y)
{
    yVal = y;
}

void Paddle::draw()
{
    image[0] = 0x8001;
    image[1] = 0x8001;


    for (int i = yVal; i < yVal + 4; i++) {
        image[0] += (int) pow ((double) 2, (double) i);
        image[1] += (int) pow ((double) 2, (double) i);
    }
}

void Ball::set_values (int x, int y, int vx, int  vy)
{

    xPos = x;
    yPos = y;
    xSpeed = vx;
    ySpeed = vy;

}

void Ball::draw ()
{
//if(image[xPos] == (int) pow ((double) 2, (double) yPos)) return;
    image[xPos] = 0x8001;
    image[xPos+1] = 0x8001;


    xPos += xSpeed;
    yPos += ySpeed;

    if (yPos == 13) ySpeed = ySpeed * -1;
    if (yPos == 1) ySpeed = ySpeed * -1;
    if (xPos == 39 || xPos == 2) {
        if (yPos <= yVal + 4 && yPos >= yVal) {
            xSpeed *= -1;
        } else {
            if(xPos == 39 && xSpeed == 1) {
                xPos = 2;
            }
            if(xPos == 2 && xSpeed == -1) {
                xPos = 39;
            }
        }
    }
    image[xPos] += (int) pow ((double) 2, (double) yPos);
    image[xPos] += (int) pow ((double) 2, (double) yPos +1);
}

Ball firstBall;
Paddle paddle;

//function to receive data from the other player
int rf_receive(char *data, uint8_t maxLength)
{
    uint8_t len = mrf.Receive((uint8_t *)data, maxLength);
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};

    if(len > 10) {
        //Remove the header and footer of the message
        for(uint8_t i = 0; i < len-2; i++) {
            if(i<8) {
                //Make sure our header is valid first
                if(data[i] != header[i])
                    return 0;
            } else {
                data[i-8] = data[i];
            }
        }

        //pc.printf("Received: %s length:%d\r\n", data, ((int)len)-10);
    }
    return ((int)len)-10;
}

//function to give data to the other player
void rf_send(char *data, uint8_t len)
{
    //We need to prepend the message with a valid ZigBee header
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};
    uint8_t *send_buf = (uint8_t *) malloc( sizeof(uint8_t) * (len+8) );

    for(uint8_t i = 0; i < len+8; i++) {
        //prepend the 8-byte header
        send_buf[i] = (i<8) ? header[i] : data[i-8];
    }
    //pc.printf("Sent: %s\r\n", send_buf+8);

    mrf.Send(send_buf, len+8);
    free(send_buf);
}

int main()
{
    speed = 1;
    // paddle.set_value(6);
    firstBall.set_values(2, 8, 1, 1);
    // clear shift and store registers initially
    HC595.ShiftByte(0x00, ShiftReg::MSBFirst);
    HC595.Latch();
    wait(0.2);
    HC595.ShiftByte(0x00, ShiftReg::MSBFirst);
    HC595.Latch();
    interrupts = 0;
    wait(0.2);
    in.mode(PullNone);              // Set the pin to Pull None mode.
    in.rise(&flip);                 // Set up the interrupt for rising edge
    buttonUp.mode(PullDown);
    buttonUp.rise(&moveUp);
    buttonDown.mode(PullDown);
    buttonDown.rise(&moveDown);
    wait(1);
    t1.start();                // start the timer
    t2.start();
    t3.start();
    t4.start();

    while(1) {

        rxLen = rf_receive(rxBuffer, 128);
        if(rxLen > 0) {
            if(!(strcmp(rxBuffer, "point"))) {


            }
            pc.printf("Received: %s\r\n", rxBuffer);
        }


//        HC595.ShiftByte(0xFF, ShiftReg::MSBFirst); HC595.Latch();
//       HC595.ShiftByte(0xCC, ShiftReg::MSBFirst);
//       HC595.ShiftByte(0xCC, ShiftReg::MSBFirst); HC595.Latch(); wait(.5);
//       HC595.ShiftByte(0x33, ShiftReg::MSBFirst);
//       HC595.ShiftByte(0x33, ShiftReg::MSBFirst); HC595.Latch(); wait(.5);
        //  shift2Bytes(0x3333); wait(.5);
        //  shift2Bytes(0xCCCC); wait(.5);
        if(t2.read_ms() >= 500 * 1/speed) {
            drawImages();
            t2.reset();

        }

        if(t3.read_ms() >=50) {
            paddle.draw();
            t3.reset();
        }

        updateImage();
    }


}

void drawImages()
{
    firstBall.draw();
}


void updateImage()
{
    float timeOn = (((float)t_period/1000000)/numFrames)*1.005;
    shift2Bytes(ESE350[counter]);
    wait(timeOn);
    counter--;
    if (counter == 0) counter =39;
}

void flip()
{
    interrupts++;
    if (interrupts == 16) {
        t_period = t1.read_us()*8;
        t1.reset();
        interrupts = 0;
    }
}



void shift2Bytes(int16_t bytes)
{
    int MSB = bytes & 0xFF;
    int LSB = (bytes>>8) & 0xFF;
    HC595.ShiftByte(MSB, ShiftReg::LSBFirst);
    HC595.ShiftByte(LSB, ShiftReg::LSBFirst);
    HC595.Latch();
}

void ShiftReg::ShiftByte
(int8_t  data
 ,BitOrd  ord
)
{
    uint8_t mask;

    if (ord == MSBFirst) mask = 0x80;
    else                 mask = 0x01;

    for (int i = 0; i < 8; i++) {
        if (data & mask) _ds = 1;
        else             _ds = 0;


        if (ord == MSBFirst) mask = mask >> 1;
        else                 mask = mask << 1;

        _sh = 0;
        _sh = 1;
    }

}

void
ShiftReg::ShiftBit
(int8_t  data
)
{
    _ds = data;
    _sh = 0;
    _sh = 1;
}

void
ShiftReg::Latch
(
)
{
    _st = 1;
    _st = 0;
}
