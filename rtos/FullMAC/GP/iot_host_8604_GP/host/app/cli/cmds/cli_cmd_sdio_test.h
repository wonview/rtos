#ifndef _CLI_CMD_SDIO_TEST_H_
#define _CLI_CMD_SDIO_TEST_H_
#include <time.h>
#include <host_apis.h>

clock_t compareClock(clock_t value);

clock_t getClock();


void cmd_sdio_test(s32 argc, char *argv[]);
void cmd_test_send_data_frame(u16 seq,u16 size);

void cmd_test_send_evt_frame(u16 size);

void sdio_rx_test_evt_cb(void *dat, u32 len);

ssv6xxx_data_result sdio_rx_test_dat_cb(void *dat, u32 len);


#endif /* _CLI_CMD_SDIO_TEST_H_ */

