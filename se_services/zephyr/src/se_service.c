/* Copyright (C) 2023 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/mhuv2_ipm.h>
#include <se_service.h>
#include <soc_memory_map.h>
#include <zephyr/logging/log.h>
#include <errno.h>
LOG_MODULE_REGISTER(se_service, CONFIG_IPM_LOG_LEVEL);

#define DT_DRV_COMPAT   alif_secure_enclave_services

#define CH_ID           0
#define SERVICE_TIMEOUT 10000
#define SYNC_TIMEOUT    100
/* MUTEX_TIMEOUT must be higher than SERVICE_TIMEOUT */
#define MUTEX_TIMEOUT   15000
#define MAX_TRIES       100

static K_SEM_DEFINE(svc_send_sem, 0, 1);
static K_SEM_DEFINE(svc_recv_sem, 0, 1);
static K_MUTEX_DEFINE(svc_mutex);


const struct device *send_dev;
const struct device *recv_dev;

/* Manufacturing data for older Ensemble Family revision <= REV_B2 */
typedef struct {
	uint8_t x_loc: 7;
	uint8_t y_loc: 7;
	uint8_t wfr_id: 5;
	uint8_t year: 6;
	uint8_t fab_id: 1;
	uint8_t week: 6;
	uint8_t lot_no: 8;
} mfg_data_v1_t;

/* Manufacturing data for Ensemble Family revision >= REV_B3 */
typedef struct {
	uint8_t x_loc: 7;
	uint8_t zero_1: 1;
	uint8_t y_loc: 7;
	uint8_t zero_2: 1;
	uint8_t wfr_id: 5;
	uint8_t zero_3: 2;
	uint8_t fab_id: 1;
	uint8_t year: 6;
	uint8_t zero_4: 2;
	uint8_t week: 6;
	uint8_t zero_5: 2;
	uint8_t lot_no: 8;
} mfg_data_v2_t;

typedef union {
	service_header_t service_header;
	get_rnd_svc_t get_rnd_svc_d;
	get_se_revision_t get_se_revision_svc_d;
	get_toc_number_svc_t get_toc_number_svc_d;
	get_device_part_svc_t get_device_part_svc_d;
	otp_data_t read_otp_svc_d;
	get_device_revision_data_t get_device_revision_data_d;
} se_service_all_svc_t;

static se_service_all_svc_t se_service_all_svc_d;

// Needed for future APIs
#if 0
static get_toc_version_svc_t get_toc_version_svc_d;
static generic_svc_t generic_svc_d;
static get_lcs_svc_t get_lcs_svc_d;
static mbedtls_trng_hardware_poll_svc_t mbedtls_trng_hardware_poll_svc_d;
static mbedtls_aes_init_svc_t mbedtls_aes_init_svc_d;
static mbedtls_aes_set_key_svc_t mbedtls_aes_set_key_svc_d;
static mbedtls_aes_crypt_svc_t mbedtls_aes_crypt_svc_d;
static mbedtls_sha_svc_t mbedtls_sha_svc_d;
static mbedtls_ccm_gcm_set_key_svc_t mbedtls_ccm_gcm_set_key_svc_d;
static mbedtls_ccm_gcm_crypt_svc_t mbedtls_ccm_gcm_crypt_svc_d;
static mbedtls_chacha20_crypt_svc_t mbedtls_chacha20_crypt_svc_d;
static mbedtls_chachapoly_crypt_svc_t mbedtls_chachapoly_crypt_svc_d;
static mbedtls_poly1305_crypt_svc_t mbedtls_poly1305_crypt_svc_d;
static mbedtls_cmac_init_setkey_svc_t mbedtls_cmac_init_setkey_svc_d;
static mbedtls_cmac_update_svc_t mbedtls_cmac_update_svc_d;
static mbedtls_cmac_finish_svc_t mbedtls_cmac_finish_svc_d;
static mbedtls_cmac_reset_svc_t mbedtls_cmac_reset_svc_d;
static process_toc_entry_svc_t process_toc_entry_svc_d;
static boot_cpu_svc_t boot_cpu_svc_d;
static control_cpu_svc_t control_cpu_svc_d;
static pinmux_svc_t pinmux_svc_d;
static pad_control_svc_t pad_control_svc_d;
static uart_write_svc_t uart_write_svc_d;
static ospi_write_key_svc_t ospi_write_key_svc_d;
static dmpu_svc_t dmpu_svc_d;
static get_toc_version_svc_t get_toc_version_svc_d;
static get_toc_via_name_svc_t get_toc_via_name_svc_d;
static get_toc_via_cpu_id_svc_t get_toc_via_cpu_id_svc_d;
static get_toc_entry_t get_toc_entry;
static get_toc_data_t get_toc_data;
static get_otp_data_t get_otp_data;
static set_services_capabilities_t set_services_capabilities;
static stop_mode_request_svc_t stop_mode_request_svc_d;
static ewic_config_request_svc_t ewic_config_request_svc_d;
static vbat_wakeup_config_request_svc_t vbat_wakeup_config_request_svc_d;
static mem_ret_config_request_svc_t mem_ret_config_request_svc_d;
static mem_power_config_request_svc_t mem_power_config_request_svc_d;
static host_cpu_clus_pwr_req_t host_cpu_clus_pwr_req;
static bsys_pwr_req_t bsys_pwr_req;
static global_standby_request_svc_t global_standby_request_svc_d;
static m55_vtor_save_request_svc_t m55_vtor_save_request_svc_d;
static clk_select_clock_source_svc_t clk_select_clock_source_svc_d;
static clk_set_enable_svc_t clk_set_enable_svc_d;
static clk_m55_set_frequency_svc_t clk_m55_set_frequency_svc_d;
static clk_select_sys_clk_source_svc_t clk_select_sys_clk_source_svc_d;
static clk_set_clk_divider_svc_t clk_set_clk_divider_svc_d;
static pll_xtal_start_svc_t pll_xtal_start_svc_d;
static pll_clkpll_start_svc_t pll_clkpll_start_svc_d;
#endif

static uint32_t global_address;

/**
 * @brief Callback API to make sure MHUv2 messages are received.
 *
 * In send_msg_to_se function, the semaphore svc_recv_sem waits for
 * SYNC_TIMEOUT/SERVICE_TIMEOUT to receive MHUv2 data from Secure enclave (SE).
 * During the SYNC_TIMEOUT/SERVICE_TIMEOUT wait, the callback_for_receive_msg
 * should release the semaphore indicating that the data sent by SE has been
 * received otherwise considered as failure to receive data.
 *
 * parameters,
 * @dev - Driver instance.
 * @ptr - pointer to received data.
 */
static void callback_for_receive_msg(const struct device *dev, uint32_t *ptr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ptr);
	k_sem_give(&svc_recv_sem);
}
/**
 * @brief Callback API to make sure MHUv2 IPM messages are sent.
 *
 * In send_msg_to_se function, the semaphore svc_send_sem waits for
 * SYNC_TIMEOUT/SERVICE_TIMEOUT after sending MHUv2 data to Secure enclave(SE).
 * During the SYNC_TIMEOUT/SERVICE_TIMEOUT wait, the callback_for_send_msg
 * should release the semaphore indicating that SE has received the data
 * otherwise data sent is considered as failure.
 *
 * parameters,
 * @dev - Driver instance
 * @ptr - pointer to sent data.
 */
static void callback_for_send_msg(const struct device *dev, uint32_t *ptr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ptr);
	k_sem_give(&svc_send_sem);
}


/**
* @brief Send data to SE through MHUv2.

* The Dcache is flushed from address 'ptr' of size
* dcache_size before sending data to make sure sent or received data are new.
* The semphores svc_recv_sem and svc_send_sem are used with timeout
* to make sure data is received or sent.
*
* parameters,
* @ptr         - placeholder for data to be sent.
* @dcache_size - size of dcache to be flused.
*
* returns,
* 0      - success.
* err    - unable to send data.
* -ETIME - semphores are timed out.
*/
static int send_msg_to_se(uint32_t *ptr, uint32_t dcache_size,
			  uint32_t timeout)
{
	int err;
	int service_id = ((service_header_t *)ptr)->hdr_service_id;
	global_address = local_to_global(ptr);
	__asm__ volatile ("dmb 0xF":::"memory");
	sys_cache_data_flush_range(ptr, dcache_size);

	err = mhuv2_ipm_send(send_dev, CH_ID, &global_address);
	if (err) {
		LOG_ERR("failed to send request for MSG(error: %d)\n", err);
		return err;
        }

	if (k_sem_take(&svc_send_sem, K_MSEC(timeout)) != 0) {
		LOG_ERR("service %d send is timed out!\n", service_id);
		k_sem_reset(&svc_send_sem);
		return -ETIME;
	}
	if (k_sem_take(&svc_recv_sem, K_MSEC(timeout)) != 0) {
		LOG_ERR("service %d response is timed out!\n", service_id);
		k_sem_reset(&svc_recv_sem);
		return -ETIME;
	}

	sys_cache_data_invd_range(ptr, dcache_size);
	return 0;
}
/**
 * @brief Synchronize with SE or wait until SE wakes up by sending
 * multiple SE heartbeat service requests.
 *
 * returns,
 * 0     - On success. SE is woken up to service SE service requests.
 * errno - Unable to get an valid response from SE.
 */
int se_service_sync(void)
{
	int err, i = 0;

	memset(&se_service_all_svc_d, 0, sizeof(se_service_all_svc_d));
	se_service_all_svc_d.service_header.hdr_service_id =
					SERVICE_MAINTENANCE_HEARTBEAT_ID;

	if (k_mutex_lock(&svc_mutex, K_MSEC(MUTEX_TIMEOUT))) {
		LOG_ERR("Unable to lock mutex (errno = %d)\n", errno);
		return errno;
	}
	while (i < MAX_TRIES) {
		err = send_msg_to_se((uint32_t *)
			&se_service_all_svc_d.service_header,
			sizeof(se_service_all_svc_d.service_header),
			SYNC_TIMEOUT);
		if (!err)
			break;
		/* SE service timed out. Increment count */
		++i;
	}
	k_mutex_unlock(&svc_mutex);
	if (i >= MAX_TRIES) {
		LOG_ERR("Failed to synchronize with SE (errno =%d)\n", err);
		return err;
	}
	return 0;
}


/**
* @brief Send heartbeat service request to SE to check if SE is alive.

* Set the service id as SERVICE_MAINTENANCE_HEARTBEAT_ID in the
* service_header and call send_msg_to_se to send the service request.
* Use svc_mutex to avoid race condition while sending service request.
*
* returns,
* 0      - success.
* err    - if unable to send service request.
* errno  - Unable to unlock mutex.
*/
int se_service_heartbeat(void)
{
	int err;

	if(k_mutex_lock(&svc_mutex,K_MSEC(MUTEX_TIMEOUT)))
	{
		LOG_ERR("Unable to lock mutex (errno = %d)\n", errno);
		return errno;
	}
	memset(&se_service_all_svc_d, 0, sizeof(se_service_all_svc_d));
	se_service_all_svc_d.service_header.hdr_service_id =
					SERVICE_MAINTENANCE_HEARTBEAT_ID;
	err = send_msg_to_se((uint32_t *)
			&se_service_all_svc_d.service_header,
			sizeof(se_service_all_svc_d.service_header),
			SYNC_TIMEOUT);
	k_mutex_unlock(&svc_mutex);
	if (err)
	{
		LOG_ERR("%s failed with %d\n", __func__, err);
		return err;
	}
	return 0;
}

/**
* @brief Send service request to SE to get random number.

* Set the service id as SERVICE_CRYPTOCELL_GET_RND in the
* service_header, set send_rnd_length with required random number length and
* call send_msg_to_se to send the service request. Use svc_mutex to
* avoid race condition while sending service request.
*
* parameters,
* @buffer - placeholder for random number.
* @length - length of requested random number.
*
* returns,
* 0        - success, buffer contains random numbers of length 'length'.
* err      - if unable to send service request.
* errno    - unable to unlock mutex.
* resp_err - error in service response for the requested service.
*/
int se_service_get_rnd_num(uint8_t *buffer, uint16_t length)
{
	int err, resp_err = -1;
	if (!buffer) {
		LOG_ERR("Invalid argument\n");
		return -EINVAL;
	}

	if (se_service_sync()) {
		LOG_ERR("SE synchronization failed (errno =%d)\n", errno);
		return errno;
	}
	if (k_mutex_lock(&svc_mutex, K_MSEC(MUTEX_TIMEOUT))) {
		LOG_ERR("Unable to lock mutex (errno = %d)\n", errno);
		return errno;
	}
	memset(&se_service_all_svc_d, 0, sizeof(se_service_all_svc_d));
	se_service_all_svc_d.get_rnd_svc_d.header.hdr_service_id =
					SERVICE_CRYPTOCELL_GET_RND;
	se_service_all_svc_d.get_rnd_svc_d.send_rnd_length = length;

	err = send_msg_to_se((uint32_t *)
			&se_service_all_svc_d.get_rnd_svc_d,
			sizeof(se_service_all_svc_d.get_rnd_svc_d),
			SERVICE_TIMEOUT);
	resp_err = se_service_all_svc_d.get_rnd_svc_d.resp_error_code;
	k_mutex_unlock(&svc_mutex);
	if (err) {
		LOG_ERR("%s failed with %d\n", __func__, err);
		return err;
	}
	if (resp_err) {
		LOG_ERR("%s: received response error = %d\n",
			__func__, resp_err);
		return resp_err;
	}
	memcpy(buffer, (uint8_t *)se_service_all_svc_d.get_rnd_svc_d.resp_rnd,
	       length);

	return 0;
}

/**
* @brief Send service request to SE to get number of table of contents (TOC).

* Set the service id as SERVICE_SYSTEM_MGMT_GET_TOC_NUMBER in the
* service_header and call send_msg_to_se to send the service request.
* Use svc_mutex to avoid race condition while sending service request.
*
* parameters,
* @ptoc - placeholder for TOC number.
*
* returns,
* 0        - success, ptoc contains number of TOC.
* err      - if unable to send service request.
* errno    - unable to unlock mutex.
* resp_err - error in service response for the requested service.
*/
int se_service_get_toc_number(uint32_t *ptoc)
{
	int err, resp_err = -1;

	if (!ptoc) {
		LOG_ERR("Invalid argument\n");
		return -EINVAL;
	}

	if (se_service_sync()) {
		LOG_ERR("SE synchronization failed (errno =%d)\n", errno);
		return errno;
	}
	if (k_mutex_lock(&svc_mutex, K_MSEC(MUTEX_TIMEOUT))) {
		LOG_ERR("Unable to lock mutex (errno = %d)\n", errno);
		return errno;
	}
	memset(&se_service_all_svc_d, 0, sizeof(se_service_all_svc_d));
	se_service_all_svc_d.get_toc_number_svc_d.header.hdr_service_id =
					SERVICE_SYSTEM_MGMT_GET_TOC_NUMBER;

	err = send_msg_to_se((uint32_t *)
		&se_service_all_svc_d.get_toc_number_svc_d,
		sizeof(se_service_all_svc_d.get_toc_number_svc_d),
		SERVICE_TIMEOUT);
	resp_err = se_service_all_svc_d.get_toc_number_svc_d.resp_error_code;
	k_mutex_unlock(&svc_mutex);
	if (err) {
		LOG_ERR("%s failed with %d\n", __func__, err);
		return err;
	}
	if (resp_err) {
		LOG_ERR("%s: received response error = %d\n",
			__func__, resp_err);
		return resp_err;
	}

	*ptoc = se_service_all_svc_d.get_toc_number_svc_d.resp_number_of_toc;

	return 0;
}

/**
* @brief Send service request to SE to get revision of SE firmware.
*
* Set the service id as SERVICE_APPLICATION_FIRMWARE_VERSION_ID in the
* service_header and call send_msg_to_se to send the service request.
* Use svc_mutex to avoid race condition while sending service request.
*
* parameters,
* @prev - placeholder for SE firmware string up to VERSION_RESPONSE_LENGTH
*        characters.
*
* returns,
* 0        - success, prev contains SE firmware string.
* err      - if unable to send service request.
* errno    - unable to unlock mutex.
* resp_err - error in service response for the requested service.
*/
int se_service_get_se_revision(uint8_t *prev)
{
	int err, resp_err = -1;

	if (!prev) {
		LOG_ERR("Invalid argument\n");
		return -EINVAL;
	}

	if (se_service_sync()) {
		LOG_ERR("SE synchronization failed (errno =%d)\n", errno);
		return errno;
	}
	if (k_mutex_lock(&svc_mutex, K_MSEC(MUTEX_TIMEOUT))) {
		LOG_ERR("Unable to lock mutex (errno = %d)\n", errno);
		return errno;
	}
	memset(&se_service_all_svc_d, 0, sizeof(se_service_all_svc_d));
	se_service_all_svc_d.get_se_revision_svc_d.header.hdr_service_id =
					SERVICE_APPLICATION_FIRMWARE_VERSION_ID;

	err = send_msg_to_se((uint32_t *)
		&se_service_all_svc_d.get_se_revision_svc_d,
		sizeof(se_service_all_svc_d.get_se_revision_svc_d),
		SERVICE_TIMEOUT);
	resp_err = se_service_all_svc_d.get_se_revision_svc_d.resp_error_code;
	k_mutex_unlock(&svc_mutex);
	if (err) {
		LOG_ERR("%s failed with %d\n", __func__, err);
		return err;
	}
	if (resp_err) {
		LOG_ERR("%s: received response error = %d\n",
			__func__, resp_err);
		return resp_err;
	}
	memcpy(prev, (uint8_t *)
	se_service_all_svc_d.get_se_revision_svc_d.resp_se_revision,
	se_service_all_svc_d.get_se_revision_svc_d.resp_se_revision_length);

	return 0;
}

/**
* @brief Send service request to SE to get device part number.
*
* Set the service id as SERVICE_SYSTEM_MGMT_GET_DEVICE_PART_NUMBER in the
* service_header and call send_msg_to_se to send the service request.
* Use svc_mutex to avoid race condition while sending service request.
*
* parameters,
* @pdev_part - placeholder for device part number.
*
* returns,
* 0        - success, pdev_part contains device part number.
* err      - if unable to send service request.
* errno    - unable to unlock mutex.
* resp_err - error in service response for the requested service.
*/
int se_service_get_device_part_number(uint32_t *pdev_part)
{
	int err, resp_err = -1;

	if (!pdev_part) {
		LOG_ERR("Invalid argument\n");
		return -EINVAL;
	}

	if (se_service_sync()) {
		LOG_ERR("SE synchronization failed (errno =%d)\n", errno);
		return errno;
	}
	if (k_mutex_lock(&svc_mutex, K_MSEC(MUTEX_TIMEOUT))) {
		LOG_ERR("Unable to lock mutex (errno = %d)\n", errno);
		return errno;
	}
	memset(&se_service_all_svc_d, 0, sizeof(se_service_all_svc_d));
	se_service_all_svc_d.get_device_part_svc_d.header.hdr_service_id =
				SERVICE_SYSTEM_MGMT_GET_DEVICE_PART_NUMBER;

	err = send_msg_to_se((uint32_t *)
		&se_service_all_svc_d.get_device_part_svc_d,
		sizeof(se_service_all_svc_d.get_device_part_svc_d),
		SERVICE_TIMEOUT);
	resp_err = se_service_all_svc_d.get_device_part_svc_d.resp_error_code;
	k_mutex_unlock(&svc_mutex);
	if (err) {
		LOG_ERR("%s failed with %d\n", __func__, err);
		return err;
	}
	if (resp_err) {
		LOG_ERR("%s: received response error = %d\n",
			__func__, resp_err);
		return resp_err;
	}
	*pdev_part =
		se_service_all_svc_d.get_device_part_svc_d.resp_device_string;

	return 0;
}

/**
* @brief Send service request to SE to read OTP which would be used as
* an unique serial number for each device
*
* Set the service id as SERVICE_SYSTEM_MGMT_READ_OTP in the
* service_header and call send_msg_to_se to send the service request.
* Use svc_mutex to avoid race condition while sending service request.
*
* parameters,
* @potp_data - placeholder for an unique serial number.
*
* returns,
* 0        - success, potp_data contains 8 bytes unique serial number.
* err      - if unable to send service request.
* errno    - unable to unlock mutex.
* resp_err - Error in service response for the requested service.
*/
int se_service_read_otp(uint32_t *potp_data)
{
	int err, resp_err = -1;
	int otp_row;

	if (!potp_data) {
		LOG_ERR("Invalid argument\n");
		return -EINVAL;
	}

	if (se_service_sync()) {
		LOG_ERR("SE synchronization failed (errno =%d)\n", errno);
		return errno;
	}
	if (k_mutex_lock(&svc_mutex, K_MSEC(MUTEX_TIMEOUT))) {
		LOG_ERR("Unable to lock mutex (errno = %d)\n", errno);
		return errno;
	}
	memset(&se_service_all_svc_d, 0, sizeof(se_service_all_svc_d));
	se_service_all_svc_d.read_otp_svc_d.header.hdr_service_id =
					SERVICE_SYSTEM_MGMT_READ_OTP;

	for(otp_row = OTP_MANUFACTURE_INFO_SERIAL_NUMBER_START ;
	    otp_row<=OTP_MANUFACTURE_INFO_SERIAL_NUMBER_END ;
	    otp_row++, potp_data++)
	{
		se_service_all_svc_d.read_otp_svc_d.send_offset = otp_row;
		err = send_msg_to_se((uint32_t *)
			&se_service_all_svc_d.read_otp_svc_d,
			sizeof(se_service_all_svc_d.read_otp_svc_d),
			SERVICE_TIMEOUT);
		resp_err = se_service_all_svc_d.read_otp_svc_d.resp_error_code;
		if (err)
		{
			k_mutex_unlock(&svc_mutex);
			LOG_ERR("%s failed with %d\n", __func__, err);
			return err;
		}
		if (resp_err) {
			k_mutex_unlock(&svc_mutex);
			LOG_ERR("%s: received response error = %d\n",
				__func__, resp_err);
			return resp_err;
		}
		*potp_data = se_service_all_svc_d.read_otp_svc_d.otp_word;
	}
	k_mutex_unlock(&svc_mutex);
	return 0;
}
/**
 * @brief Send service request to SE to get device data
 *
 * Set the service id as SERVICE_SYSTEM_MGMT_GET_DEVICE_REVISION_DATA in the
 * service_header and call send_msg_to_se to send the service request.
 * Use svc_mutex to avoid race condition while sending service request.
 *
 * parameters,
 * @pdev_data - placeholder for device data.
 *
 * returns,
 * 0        - success, pdev_data contains device data containing
 *            SoC revision, SoC part number, various keys,
 *            firmware version, wounding data, DCU settings,
 *            manufacturing data, serial number, SoC lifecycle state.
 * err      - if unable to send service request.
 * errno    - unable to unlock mutex.
 * resp_err - Error in service response for the requested service.
 */
int se_service_system_get_device_data(get_device_revision_data_t *pdev_data)
{
	int err, resp_err = -1;

	if (!pdev_data) {
		LOG_ERR("Invalid argument\n");
		return -EINVAL;
	}

	if (se_service_sync()) {
		LOG_ERR("SE synchronization failed (errno =%d)\n", errno);
		return errno;
	}
	if (k_mutex_lock(&svc_mutex, K_MSEC(MUTEX_TIMEOUT))) {
		LOG_ERR("Unable to lock mutex (errno = %d)\n", errno);
		return errno;
	}
	memset(&se_service_all_svc_d, 0, sizeof(se_service_all_svc_d));
	se_service_all_svc_d.get_device_revision_data_d.header.hdr_service_id
			= SERVICE_SYSTEM_MGMT_GET_DEVICE_REVISION_DATA;

	err = send_msg_to_se((uint32_t *)
		&se_service_all_svc_d.get_device_revision_data_d,
		sizeof(se_service_all_svc_d.get_device_revision_data_d),
		SERVICE_TIMEOUT);
	resp_err =
	se_service_all_svc_d.get_device_revision_data_d.resp_error_code;
	if (err) {
		k_mutex_unlock(&svc_mutex);
		LOG_ERR("%s failed with %d\n", __func__, err);
		return err;
	}
	if (resp_err) {
		k_mutex_unlock(&svc_mutex);
		LOG_ERR("%s: received response error = %d\n",
			__func__, resp_err);
		return resp_err;
	}
	pdev_data->revision_id =
		se_service_all_svc_d.get_device_revision_data_d.revision_id;
	memcpy((uint8_t *)pdev_data->SerialN,
	(uint8_t *)se_service_all_svc_d.get_device_revision_data_d.SerialN,
	sizeof(pdev_data->SerialN));
	memcpy((uint8_t *)pdev_data->ALIF_PN,
	(uint8_t *)se_service_all_svc_d.get_device_revision_data_d.ALIF_PN,
	sizeof(pdev_data->ALIF_PN));
	memcpy((uint8_t *)pdev_data->HBK0,
	(uint8_t *)se_service_all_svc_d.get_device_revision_data_d.HBK0,
	sizeof(pdev_data->HBK0));
	memcpy((uint8_t *)pdev_data->DCU,
	(uint8_t *)se_service_all_svc_d.get_device_revision_data_d.DCU,
	sizeof(pdev_data->DCU));
	memcpy((uint8_t *)pdev_data->config,
	(uint8_t *)se_service_all_svc_d.get_device_revision_data_d.config,
	sizeof(pdev_data->config));
	memcpy((uint8_t *)pdev_data->HBK1,
	(uint8_t *)se_service_all_svc_d.get_device_revision_data_d.HBK1,
	sizeof(pdev_data->HBK1));
	memcpy((uint8_t *)pdev_data->HBK_FW,
	(uint8_t *)se_service_all_svc_d.get_device_revision_data_d.HBK_FW,
	sizeof(pdev_data->HBK_FW));
	memcpy((uint8_t *)pdev_data->MfgData,
	(uint8_t *)se_service_all_svc_d.get_device_revision_data_d.MfgData,
	sizeof(pdev_data->MfgData));

	pdev_data->LCS = se_service_all_svc_d.get_device_revision_data_d.LCS;

	k_mutex_unlock(&svc_mutex);
	return 0;
}

/**
 * @brief   Pack manufacturing data into 40 bits for EUI-64
 */
static void get_eui64_extension(mfg_data_v1_t *p_mfg_data, uint8_t *p_data_out)
{
	uint8_t seven_bits_1 = p_mfg_data->x_loc;
	uint8_t seven_bits_2 = p_mfg_data->y_loc;
	uint8_t six_bits_3 = (p_mfg_data->wfr_id << 1) | p_mfg_data->fab_id;
	uint8_t six_bits_4 = p_mfg_data->year;
	uint8_t six_bits_5 = p_mfg_data->week;
	uint8_t eight_bits = p_mfg_data->lot_no;

	/* x x x x x x x y */
	*p_data_out = (seven_bits_1 << 1) | ((seven_bits_2 & 0x40) >> 6);
	p_data_out++;
	/* y y y y y y wf wf */
	*p_data_out = ((seven_bits_2 & 0x3F) << 2) | ((six_bits_3 & 0x30) >> 4);
	p_data_out++;
	/* wf wf wf f yr yr yr yr */
	*p_data_out = ((six_bits_3 & 0x0F) << 4) | ((six_bits_4 & 0x3C) >> 2);
	p_data_out++;
	/* yr yr wk wk wk wk wk wk */
	*p_data_out = ((six_bits_4 & 0x03) << 6) | six_bits_5;
	p_data_out++;
	*p_data_out = eight_bits;
}

/**
 * @brief   Pack manufacturing data into 24 bits for EUI-48
 */
static void get_eui48_extension(mfg_data_v1_t *p_mfg_data, uint8_t *p_data_out)
{
	uint8_t six_bits_1 = p_mfg_data->x_loc & 0x3F;
	uint8_t six_bits_2 = p_mfg_data->y_loc & 0x3F;
	uint8_t six_bits_3 = (p_mfg_data->wfr_id << 1) | (p_mfg_data->lot_no & 0x1);
	uint8_t six_bits_4 = p_mfg_data->week;

	/* x x x x x x y y */
	*p_data_out = (six_bits_1 << 2) | ((six_bits_2 & 0x30) >> 4);
	p_data_out++;
	/* y y y y wf wf wf wf */
	*p_data_out = ((six_bits_2 & 0x0F) << 4) | ((six_bits_3 & 0x3C) >> 2);
	p_data_out++;
	/* wf lt wk wk wk wk wk wk */
	*p_data_out = ((six_bits_3 & 0x03) << 6) | six_bits_4;
}

static void se_service_manufacture_data_parse(get_device_revision_data_t *device_data,
					      mfg_data_v1_t *mfg_data)
{
	if (IS_ENABLED(CONFIG_SOC_FAMILY_ENSEMBLE) && device_data->revision_id < 0x0000b300) {
		mfg_data_v1_t *p_mfg_data = (mfg_data_v1_t *)device_data->MfgData;

		mfg_data->fab_id = p_mfg_data->fab_id;
		mfg_data->lot_no = p_mfg_data->lot_no;
		mfg_data->week = p_mfg_data->week;
		mfg_data->wfr_id = p_mfg_data->wfr_id;
		mfg_data->x_loc = p_mfg_data->x_loc;
		mfg_data->y_loc = p_mfg_data->y_loc;
		mfg_data->year = p_mfg_data->year;
	} else {
		/* Ensemble(rev>=B3) families use new Manufacture data model */
		mfg_data_v2_t *p_mfg_data = (mfg_data_v2_t *)device_data->MfgData;

		mfg_data->fab_id = p_mfg_data->fab_id;
		mfg_data->lot_no = p_mfg_data->lot_no;
		mfg_data->week = p_mfg_data->week;
		mfg_data->wfr_id = p_mfg_data->wfr_id;
		mfg_data->x_loc = p_mfg_data->x_loc;
		mfg_data->y_loc = p_mfg_data->y_loc;
		mfg_data->year = p_mfg_data->year;
	}
}

/**
 * @brief Calculate unique extension values for EUI-48 or EUI-64
 *
 * @fn    uint32_t se_system_get_eui_extension(
 *                                bool is_eui48,
 *                                uint8_t * eui_extension)
 *
 * @param is_eui48       Specify whether EUI-48 or EUI-64 extension is requested
 * @param eui_extension  Buffer to store the calculated extension
 * @return
 */
int se_system_get_eui_extension(bool is_eui48, uint8_t *eui_extension)
{
	get_device_revision_data_t device_data;
	mfg_data_v1_t p_mfg_dat;
	int ret;

	ret = se_service_system_get_device_data(&device_data);
	if (ret) {
		return ret;
	}

	se_service_manufacture_data_parse(&device_data, &p_mfg_dat);

	/* Use Manufactured data  */
	if (is_eui48) {
		get_eui48_extension(&p_mfg_dat, eui_extension);
	} else {
		get_eui64_extension(&p_mfg_dat, eui_extension);
	}

	return ret;
}

/**
* @brief Check the MHUv2 devices are ready and initialize callbacks for
* the recevied and send data.
*
* returns,
* 0       - success.
* -ENODEV - if the MHUv2 devices are not ready.
*/
static int se_service_mhuv2_nodes_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	send_dev = DEVICE_DT_GET_OR_NULL(DT_PHANDLE
				(DT_NODELABEL(se_service), mhuv2_send_node));
	recv_dev = DEVICE_DT_GET_OR_NULL(DT_PHANDLE
				(DT_NODELABEL(se_service), mhuv2_recv_node));

	if (!device_is_ready(recv_dev) || !device_is_ready(send_dev)) {
	        printk("MHU devices not ready\n");
	        return -ENODEV;
	}
	mhuv2_ipm_rb(recv_dev, callback_for_receive_msg, NULL);
	mhuv2_ipm_rb(send_dev, callback_for_send_msg, NULL);
	return 0;
}

SYS_INIT(se_service_mhuv2_nodes_init, POST_KERNEL,
	 CONFIG_SE_SERVICE_INIT_PRIORITY);
