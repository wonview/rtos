#ifndef _AP_MLME_H_
#define _AP_MLME_H_

#include <pbuf.h>

/**
 *  Define MLME error numbers:
 */
#define AP_MLME_OK                                  0
#define AP_MLME_FAILED                             -1
#define AP_MLME_TIMEOUT                            -2



struct ApInfo;

typedef s32 (*AP_MGMT80211_RxHandler)(struct ApInfo*, struct ieee80211_mgmt *mgmt, u16 len);

extern AP_MGMT80211_RxHandler AP_RxHandler[];




int i802_sta_deauth(const u8 *own_addr, const u8 *addr, int reason);
int i802_sta_disassoc(const u8 *own_addr, const u8 *addr, int reason);
int nl80211_poll_client(const u8 *own_addr, const u8 *addr, int qos);



#endif /* _AP_MLME_H_ */

