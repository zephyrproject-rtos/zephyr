#ifndef LSM6DS3_H
#define LSM6DS3_H

typedef struct {
    struct {
        int16_t x;
        int16_t y;
        int16_t z;
    } accel;
    struct {
        int16_t x;
        int16_t y;
        int16_t z;
    } gyro;
} lsm6ds3_data_t;

int lsm6ds3_init(void);
int lsm6ds3_read(lsm6ds3_data_t *data);

#endif