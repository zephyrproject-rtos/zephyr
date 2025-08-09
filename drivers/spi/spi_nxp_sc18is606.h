#ifdef __cplusplus
extern "C" {
#endif

/* @brief Claim the the SC18IS606 bridge
 *
 * @warning After calling this routine, the device cannot be used by any other thread
 * until the calling bridge releases it with the counterpart function of this.
 *
 * @param  dev SC18IS606  device
 * @retval 0 Device is claimed
 * @retval -EBUSY The device cannnot be claimed
 */
int nxp_sc18is606_claim(const struct device *dev);

/* @brief Release the SC18IS606 bridge
 *
 * @warning this routine can only be called once a device has been locked
 *
 * @param dev SC18IS606 bridge
 *
 * @retval 0  Device is released
 * @retval -EPERM The current thread has not claimed this so cannot release it
 * @retval -EINVAL The device has no locks on it.
 */
int nxp_sc18is606_release(const struct  device *dev);



#ifdef __cplusplus
}
#endif
