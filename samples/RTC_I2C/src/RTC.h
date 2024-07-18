
#define CLOCK_H24 0
#define CLOCK_H12 1

#define HOUR_AM 0
#define HOUR_PM 1
#define HOUR_24 2

#define DS3231_ADDR 0x68


/*ftypes*/
uint8_t bitSet(uint8_t data,uint8_t shift);
uint8_t bitClear(uint8_t data,uint8_t shift);
uint8_t bitRead(uint8_t data,uint8_t shift);
uint8_t bitWrite(uint8_t data,uint8_t shift,uint8_t bit);
void DS3231_setHourMode(const struct device *dev,uint8_t slave_address,uint8_t h_mode);
uint8_t DS3231_getHourMode(const struct device *dev,uint8_t slave_address);
void DS3231_setMeridiem(const struct device *dev,uint8_t slave_address,uint8_t meridiem);
uint8_t DS3231_getMeridiem(const struct device *dev,uint8_t slave_address);
uint8_t DS3231_getSeconds(const struct device *dev,uint8_t slave_address);
void DS3231_setSeconds(const struct device *dev,uint8_t slave_address,uint8_t seconds);
uint8_t DS3231_getMinutes(const struct device *dev,uint8_t slave_address);
void DS3231_setMinutes(const struct device *dev,uint8_t slave_address,uint8_t minutes);
uint8_t DS3231_getHours(const struct device *dev,uint8_t slave_address);
void  DS3231_setHours(const struct device *dev,uint8_t slave_address,uint8_t hours);
uint8_t DS3231_getWeek(const struct device *dev,uint8_t slave_address);
void DS3231_setWeek(const struct device *dev,uint8_t slave_address,uint8_t week);
void DS3231_updateWeek(const struct device *dev,uint8_t slave_address);
uint8_t DS3231_getDay(const struct device *dev,uint8_t slave_address);
void DS3231_setDay(const struct device *dev,uint8_t slave_address,uint8_t day);
uint8_t DS3231_getMonth(const struct device *dev,uint8_t slave_address);
void DS3231_setMonth(const struct device *dev,uint8_t slave_address,uint8_t month);
uint16_t DS3231_getYear(const struct device *dev,uint8_t slave_address);
void DS3231_setYear(const struct device *dev,uint8_t slave_address,uint16_t year);
void DS3231_setTime(const struct device *dev,uint8_t slave_address,uint8_t hours, uint8_t minutes, uint8_t seconds);
void DS3231_setDate(const struct device *dev,uint8_t slave_address,uint8_t day, uint8_t month, uint16_t year);
void DS3231_setDateTime(const struct device *dev,uint8_t slave_address,char* date, char* time);
uint8_t bcd2bin_in(uint8_t val);
uint8_t bin2bcd_in(uint8_t val);
