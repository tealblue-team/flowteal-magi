#define SPRINKLER_SOCKET_MAX_BUF_LEN 1280

struct sprinkler_context {
	int sock;
	uint8_t payload[SPRINKLER_SOCKET_MAX_BUF_LEN];
	uint8_t resp[SPRINKLER_SOCKET_MAX_BUF_LEN];
};

int sprinkler_connect(struct sprinkler_context *ctx);
int sprinkler_http_push(struct sprinkler_context *ctx,
		     http_response_cb_t resp_cb);
