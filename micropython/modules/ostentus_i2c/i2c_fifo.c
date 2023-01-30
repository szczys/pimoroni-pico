#include <stdio.h>
#include <string.h>
#include "i2c_multi.h"
#include "pico/stdlib.h"

#define OSTENTUS_CMD_ADDR 0x12
#define OSTENTUS_DAT_ADDR 0x13
PIO pio = pio0;
uint pin = 4;
#define BUFFER_SIZE 64
uint8_t buffer[BUFFER_SIZE] = {0};

#define FIFO_SIZE 32
#define FIFO_DATA_OFFSET 2 // cmd addr, data_len
uint8_t _cur_op = 0;
uint8_t _fifo[FIFO_SIZE][BUFFER_SIZE];
uint8_t _has_data;
uint8_t _cmd_idx;
uint8_t _dat_idx;
uint8_t _head;
uint8_t _tail;

void i2c_receive_handler(uint8_t data, bool is_address)
{
    if ((_head == _tail) && _has_data) {
        //Buffer is full, do nothing.
        return;
    }
    if (is_address) {
        _cur_op = data;
        printf("\nAddress: %X, receiving...", data);
    }
    else {
        if ((_cur_op == OSTENTUS_CMD_ADDR) && (_cmd_idx == 0)) {
            _fifo[_tail][_cmd_idx++] = data;
        } else if (_cur_op == OSTENTUS_DAT_ADDR) {
            if (_dat_idx < BUFFER_SIZE) {
                _fifo[_tail][_dat_idx] = data;
                ++_dat_idx;
            }
        }
    }
}

/* Currently unused, this is the example from the i2c_multi lib */
// https://github.com/dgatf/I2C-slave-multi-address-RP2040
void i2c_request_handler(uint8_t address)
{
    printf("\nAddress: %X, request...", address);

    switch (address)
    {
    case 0x70:
        buffer[0] = 0x10;
        buffer[1] = 0x11;
        buffer[2] = 0x12;
        break;
    case 0x71:
        sprintf((char *)buffer, "Hello, I'm %X", address);
        break;
    }
}

void i2c_stop_handler(uint8_t length)
{
    if (_cur_op != OSTENTUS_CMD_ADDR) {
        return;
    }
    //Write data len
    _fifo[_tail][FIFO_DATA_OFFSET-1] = _dat_idx-FIFO_DATA_OFFSET;
    //Indicate there's info in the fifo
    ++_has_data;
    if (++_tail >= FIFO_SIZE) {
        _tail = 0;
    }

    //Reset to defaults
    if (_head != _tail) {
        memset(_fifo[_tail], 0, BUFFER_SIZE);
    }
    _cmd_idx = 0;
    _dat_idx = FIFO_DATA_OFFSET;
    _cur_op = 0;
}

int fifo_pop(uint8_t * r_buf) {
    if (_has_data == 0) {
        return -1;
    }
    memcpy(r_buf, _fifo[_head], BUFFER_SIZE);
    ++_head;
    if (_head >= FIFO_SIZE) {
        _head = 0;
    }
    --_has_data;
    return 0;
}

bool fifo_has_data(void) {
    if (_has_data) {
        return true;
    }
    return false;
}

void fifo_init(void)
{
    _cur_op = 0;
    _has_data = 0;
    _cmd_idx = 0;
    _dat_idx = 0;
    _head = 0;
    _tail = 0;
    memset(_fifo[_head], 0, BUFFER_SIZE);

    i2c_multi_init(pio, pin);
    i2c_multi_enable_address(OSTENTUS_CMD_ADDR);
    i2c_multi_enable_address(OSTENTUS_DAT_ADDR);
    i2c_multi_set_receive_handler(i2c_receive_handler);
    i2c_multi_set_request_handler(i2c_request_handler);
    i2c_multi_set_stop_handler(i2c_stop_handler);
    i2c_multi_set_write_buffer(buffer);
}
