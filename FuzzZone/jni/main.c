#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "qseecom.h"

struct __attribute__((packed)) qseecom_send_raw_scm_req {
        uint32_t svc_id;
        uint32_t cmd_id;
        void *cmd_req_buf; /* in */
        unsigned int cmd_req_len; /* in */
        void *resp_buf; /* in/out */
        unsigned int resp_len; /* in/out */
};

struct __attribute__((packed)) qseecom_send_atomic_scm_req {
    uint32_t svc_id;
    uint32_t cmd_id;
    uint32_t num_args;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
};


#define QSEECOM_IOCTL_SEND_RAW_SCM \
        _IOWR(QSEECOM_IOC_MAGIC, 21, struct qseecom_send_raw_scm_req)

#define QSEECOM_IOCTL_SEND_ATOMIC_SCM \
	_IOWR(QSEECOM_IOC_MAGIC, 24, struct qseecom_send_atomic_scm_req)

int main(int argc, char **argv) {

	//Reading the command-line arguments
	if (argc < 2) {
		printf("USAGE: fuzz_zone <MODE>\n");
		return -EINVAL;
	}
	char* mode = argv[1];

        //Opening the QSEECOM device
        int fd = open("/dev/qseecom", O_RDONLY);
        if (fd < 0) {
                perror("Failed to open /dev/qseecom");
                return -errno;
        }
        printf("FD: %d\n", fd);

	
	//Checking if this is an atomic call
	if (strstr(mode, "reg") == mode) {

		//Reading the arguments from the user
		if (argc < 4) {
			printf("USAGE: %s reg <SVC_ID> <CMD_ID> <NUM_ARGS> <HEX ARGS...>\n", argv[0]);
			return -EINVAL;
		}
		struct qseecom_send_atomic_scm_req req;
		req.svc_id = atoi(argv[2]);
		req.cmd_id = atoi(argv[3]);
		req.num_args = atoi(argv[4]);
		if (req.num_args > 4) {
			printf("Illegal number of arguments supplied: %d\n", req.num_args);
			return -EINVAL;
		}
		if (req.num_args > 0)
			req.arg1 = (unsigned)strtoll(argv[5], NULL, 16);
		if (req.num_args > 1)
			req.arg2 = (unsigned)strtoll(argv[6], NULL, 16);
                if (req.num_args > 2)
                        req.arg3 = (unsigned)strtoll(argv[7], NULL, 16);
		if (req.num_args > 3)
			req.arg4 = (unsigned)strtoll(argv[8], NULL, 16);
		int res = ioctl(fd, QSEECOM_IOCTL_SEND_ATOMIC_SCM, &req);
                printf("IOCTL RES: %u\n", (unsigned)res);
		if (res < 0) {
			perror("Failed to send ioctl");
		}

	}	

	//Checking if this is a raw call
	else if (strstr(mode, "raw") == mode) {

		if (argc != 6) {
			printf("USAGE: %s raw <SVC_ID> <CMD_ID> <REQ_BUF> <RESP_LEN>\n", argv[0]);
			return -EINVAL;
		}
	        uint32_t svc_id = atoi(argv[2]);
        	uint32_t cmd_id = atoi(argv[3]);
	        char* hex_cmd_buf = argv[4];
		uint32_t resp_len = atoi(argv[5]);

        	//Converting the hex string to a binary string
	        unsigned cmd_req_len = strlen(hex_cmd_buf)/2;
	        char* bin_cmd_req = malloc(cmd_req_len);
	        for (int i=0; i<cmd_req_len; i++)
	                sscanf(hex_cmd_buf+i*2,"%2hhx", bin_cmd_req+i);


		//Sending the request
		struct qseecom_send_raw_scm_req raw_req;
		raw_req.svc_id = svc_id;
		raw_req.cmd_id = cmd_id;
		raw_req.cmd_req_len = cmd_req_len;
		raw_req.cmd_req_buf = bin_cmd_req;
		raw_req.resp_buf = malloc(resp_len);
		memset(raw_req.resp_buf, 'B', resp_len); //Visible garbage to see the actual change
		raw_req.resp_len = resp_len;
		int res = ioctl(fd, QSEECOM_IOCTL_SEND_RAW_SCM, &raw_req);
		if (res < 0) {
			perror("Failed to send raw SCM ioctl");
			return -errno;
		}
		printf("IOCTL RES: %d\n", res);
		
		//Printing the response buffer
		printf("Response Buffer:\n");
		uint32_t i;
		for (i=0; i<raw_req.resp_len; i++)
			printf("%02X", ((unsigned char*)raw_req.resp_buf)[i]);
		printf("\n");
	}

	else {
		printf("Unknown mode %s!\n", mode);
		return -EINVAL;
	}	
}
