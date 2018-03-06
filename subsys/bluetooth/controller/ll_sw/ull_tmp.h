/* NOTE: Definitions used between Thread and ULL implementations */

int ull_tmp_enable(u16_t handle);
int ull_tmp_disable(u16_t handle);

int ull_tmp_data_send(u16_t handle, u8_t size, u8_t *data);
