#ifndef DEVICE_HTTP_CLIENT_H_
#define DEVICE_HTTP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif






int send_telegram_message(const char *token, const char *chat_id, const char *message);
void create_telegram_message(unsigned device_state);
int device_update_time();








#ifdef __cplusplus
}
#endif









#endif