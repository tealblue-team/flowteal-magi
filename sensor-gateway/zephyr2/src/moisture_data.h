#include <zephyr/data/json.h>

struct moisture {
	char *sid;
	int msgid;
	int adc;
	int V;
	int vwc;
	};
struct json_obj_descr moisture_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct moisture, sid, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct moisture, msgid, JSON_TOK_NUMBER),
	//JSON_OBJ_DESCR_PRIM(struct moisture, adc, JSON_TOK_NUMBER), 
	//JSON_OBJ_DESCR_PRIM(struct moisture, V, JSON_TOK_NUMBER), 
	JSON_OBJ_DESCR_PRIM(struct moisture, vwc, JSON_TOK_NUMBER)
	};


int expected_ret_code = (1 << ARRAY_SIZE(moisture_descr)) - 1;