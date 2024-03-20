#ifndef _BUFF_UTILS_H_
#define _BUFF_UTILS_H_

unsigned char getBuffOne(unsigned char *buffer, unsigned buffer_size, unsigned *offset);
unsigned short getBuffTwo(unsigned char *buffer, unsigned buffer_size, unsigned *offset);
unsigned int getBuffFour(unsigned char *buffer, unsigned buffer_size, unsigned *offset);
long unsigned int getBuffEight(unsigned char *buffer, unsigned buffer_size, unsigned *offset);

#endif // _BUFF_UTILS_H_
