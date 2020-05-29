/*
* Copyright (c) 2016 Google Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holder nor the names of its
* contributors may be used to endorse or promote products derived from this
* software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __INCLUDE_NUTTX_CAMERA_PROPERTIES__
#define __INCLUDE_NUTTX_CAMERA_PROPERTIES__

/**
 * Define the property tag sections.
 *
 * All tags ids are 16 bits long and are assembled as:
 * id = (uint16_t)((section << 8) | tag_id);
 */
#define GB_CAM_COLOR_CORRECTION_SECTION                              0x000
#define GB_CAM_CONTROL_SECTION                                       0x100
#define GB_CAM_DEMOSAIC_SECTION                                      0x200
#define GB_CAM_EDGE_SECTION                                          0x300
#define GB_CAM_FLASH_SECTION                                         0x400
#define GB_CAM_FLASH_INFO_SECTION                                    0x500
#define GB_CAM_HOT_PIXEL_SECTION                                     0x600
#define GB_CAM_JPEG_SECTION                                          0x700
#define GB_CAM_LENS_SECTION                                          0x800
#define GB_CAM_LENS_INFO_SECTION                                     0x900
#define GB_CAM_NOISE_REDUCTION_SECTION                               0xa00
#define GB_CAM_QUIRKS_SECTION                                        0xb00
#define GB_CAM_REQUEST_SECTION                                       0xc00
#define GB_CAM_SCALER_SECTION                                        0xd00
#define GB_CAM_SENSOR_SECTION                                        0xe00
#define GB_CAM_SENSOR_INFO_SECTION                                   0xf00
#define GB_CAM_SHADING_SECTION                                       0x1000
#define GB_CAM_STATISTICS_SECTION                                    0x1100
#define GB_CAM_STATISTICS_INFO_SECTION                               0x1200
#define GB_CAM_TONEMAP_SECTION                                       0x1300
#define GB_CAM_LED_SECTION                                           0x1400
#define GB_CAM_INFO_SECTION                                          0x1500
#define GB_CAM_BLACK_LEVEL_SECTION                                   0x1600
#define GB_CAM_SYNC_SECTION                                          0x1700
#define GB_CAM_REPROCESS_SECTION                                     0x1800
#define GB_CAM_DEPTH_SECTION                                         0x1900
#define GB_CAM_SECTION_COUNT                                         0x1a00
#define GB_CAM_ARA_SECTION                                           0x7f00
#define GB_CAM_VENDOR_SECTION                                        0x8000

/**
 * Define the property tag ids
 */
#define GB_CAM_COLOR_CORRECTION_MODE                                 (GB_CAM_COLOR_CORRECTION_SECTION | 0x00)
#define GB_CAM_COLOR_CORRECTION_TRANSFORM                            (GB_CAM_COLOR_CORRECTION_SECTION | 0x01)
#define GB_CAM_COLOR_CORRECTION_GAINS                                (GB_CAM_COLOR_CORRECTION_SECTION | 0x02)
#define GB_CAM_COLOR_CORRECTION_ABERRATION_MODE                      (GB_CAM_COLOR_CORRECTION_SECTION | 0x03)
#define GB_CAM_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES           (GB_CAM_COLOR_CORRECTION_SECTION | 0x04)
#define GB_CAM_CONTROL_AE_ANTIBANDING_MODE                           (GB_CAM_CONTROL_SECTION | 0x00)
#define GB_CAM_CONTROL_AE_EXPOSURE_COMPENSATION                      (GB_CAM_CONTROL_SECTION | 0x01)
#define GB_CAM_CONTROL_AE_LOCK                                       (GB_CAM_CONTROL_SECTION | 0x02)
#define GB_CAM_CONTROL_AE_MODE                                       (GB_CAM_CONTROL_SECTION | 0x03)
#define GB_CAM_CONTROL_AE_REGIONS                                    (GB_CAM_CONTROL_SECTION | 0x04)
#define GB_CAM_CONTROL_AE_TARGET_FPS_RANGE                           (GB_CAM_CONTROL_SECTION | 0x05)
#define GB_CAM_CONTROL_AE_PRECAPTURE_TRIGGER                         (GB_CAM_CONTROL_SECTION | 0x06)
#define GB_CAM_CONTROL_AF_MODE                                       (GB_CAM_CONTROL_SECTION | 0x07)
#define GB_CAM_CONTROL_AF_REGIONS                                    (GB_CAM_CONTROL_SECTION | 0x08)
#define GB_CAM_CONTROL_AF_TRIGGER                                    (GB_CAM_CONTROL_SECTION | 0x09)
#define GB_CAM_CONTROL_AWB_LOCK                                      (GB_CAM_CONTROL_SECTION | 0x0a)
#define GB_CAM_CONTROL_AWB_MODE                                      (GB_CAM_CONTROL_SECTION | 0x0b)
#define GB_CAM_CONTROL_AWB_REGIONS                                   (GB_CAM_CONTROL_SECTION | 0x0c)
#define GB_CAM_CONTROL_CAPTURE_INTENT                                (GB_CAM_CONTROL_SECTION | 0x0d)
#define GB_CAM_CONTROL_EFFECT_MODE                                   (GB_CAM_CONTROL_SECTION | 0x0e)
#define GB_CAM_CONTROL_MODE                                          (GB_CAM_CONTROL_SECTION | 0x0f)
#define GB_CAM_CONTROL_SCENE_MODE                                    (GB_CAM_CONTROL_SECTION | 0x10)
#define GB_CAM_CONTROL_VIDEO_STABILIZATION_MODE                      (GB_CAM_CONTROL_SECTION | 0x11)
#define GB_CAM_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES                (GB_CAM_CONTROL_SECTION | 0x12)
#define GB_CAM_CONTROL_AE_AVAILABLE_MODES                            (GB_CAM_CONTROL_SECTION | 0x13)
#define GB_CAM_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES                (GB_CAM_CONTROL_SECTION | 0x14)
#define GB_CAM_CONTROL_AE_COMPENSATION_RANGE                         (GB_CAM_CONTROL_SECTION | 0x15)
#define GB_CAM_CONTROL_AE_COMPENSATION_STEP                          (GB_CAM_CONTROL_SECTION | 0x16)
#define GB_CAM_CONTROL_AF_AVAILABLE_MODES                            (GB_CAM_CONTROL_SECTION | 0x17)
#define GB_CAM_CONTROL_AVAILABLE_EFFECTS                             (GB_CAM_CONTROL_SECTION | 0x18)
#define GB_CAM_CONTROL_AVAILABLE_SCENE_MODES                         (GB_CAM_CONTROL_SECTION | 0x19)
#define GB_CAM_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES           (GB_CAM_CONTROL_SECTION | 0x1a)
#define GB_CAM_CONTROL_AWB_AVAILABLE_MODES                           (GB_CAM_CONTROL_SECTION | 0x1b)
#define GB_CAM_CONTROL_MAX_REGIONS                                   (GB_CAM_CONTROL_SECTION | 0x1c)
#define GB_CAM_CONTROL_SCENE_MODE_OVERRIDES                          (GB_CAM_CONTROL_SECTION | 0x1d)
#define GB_CAM_CONTROL_AE_PRECAPTURE_ID                              (GB_CAM_CONTROL_SECTION | 0x1e)
#define GB_CAM_CONTROL_AE_STATE                                      (GB_CAM_CONTROL_SECTION | 0x1f)
#define GB_CAM_CONTROL_AF_STATE                                      (GB_CAM_CONTROL_SECTION | 0x20)
#define GB_CAM_CONTROL_AF_TRIGGER_ID                                 (GB_CAM_CONTROL_SECTION | 0x21)
#define GB_CAM_CONTROL_AWB_STATE                                     (GB_CAM_CONTROL_SECTION | 0x22)
#define GB_CAM_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS     (GB_CAM_CONTROL_SECTION | 0x23)
#define GB_CAM_CONTROL_AE_LOCK_AVAILABLE                             (GB_CAM_CONTROL_SECTION | 0x24)
#define GB_CAM_CONTROL_AWB_LOCK_AVAILABLE                            (GB_CAM_CONTROL_SECTION | 0x25)
#define GB_CAM_CONTROL_AVAILABLE_MODES                               (GB_CAM_CONTROL_SECTION | 0x26)
#define GB_CAM_DEMOSAIC_MODE                                         (GB_CAM_DEMOSAIC_SECTION | 0x00)
#define GB_CAM_EDGE_MODE                                             (GB_CAM_EDGE_SECTION | 0x00)
#define GB_CAM_EDGE_STRENGTH                                         (GB_CAM_EDGE_SECTION | 0x01)
#define GB_CAM_EDGE_AVAILABLE_EDGE_MODES                             (GB_CAM_EDGE_SECTION | 0x02)
#define GB_CAM_FLASH_FIRING_POWER                                    (GB_CAM_FLASH_SECTION | 0x00)
#define GB_CAM_FLASH_FIRING_TIME                                     (GB_CAM_FLASH_SECTION | 0x01)
#define GB_CAM_FLASH_MODE                                            (GB_CAM_FLASH_SECTION | 0x02)
#define GB_CAM_FLASH_COLOR_TEMPERATURE                               (GB_CAM_FLASH_SECTION | 0x03)
#define GB_CAM_FLASH_MAX_ENERGY                                      (GB_CAM_FLASH_SECTION | 0x04)
#define GB_CAM_FLASH_STATE                                           (GB_CAM_FLASH_SECTION | 0x05)
#define GB_CAM_FLASH_INFO_AVAILABLE                                  (GB_CAM_FLASH_INFO_SECTION | 0x00)
#define GB_CAM_FLASH_INFO_CHARGE_DURATION                            (GB_CAM_FLASH_INFO_SECTION | 0x01)
#define GB_CAM_HOT_PIXEL_MODE                                        (GB_CAM_HOT_PIXEL_SECTION | 0x00)
#define GB_CAM_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES                   (GB_CAM_HOT_PIXEL_SECTION | 0x01)
#define GB_CAM_JPEG_GPS_COORDINATES                                  (GB_CAM_JPEG_SECTION | 0x00)
#define GB_CAM_JPEG_GPS_PROCESSING_METHOD                            (GB_CAM_JPEG_SECTION | 0x01)
#define GB_CAM_JPEG_GPS_TIMESTAMP                                    (GB_CAM_JPEG_SECTION | 0x02)
#define GB_CAM_JPEG_ORIENTATION                                      (GB_CAM_JPEG_SECTION | 0x03)
#define GB_CAM_JPEG_QUALITY                                          (GB_CAM_JPEG_SECTION | 0x04)
#define GB_CAM_JPEG_THUMBNAIL_QUALITY                                (GB_CAM_JPEG_SECTION | 0x05)
#define GB_CAM_JPEG_THUMBNAIL_SIZE                                   (GB_CAM_JPEG_SECTION | 0x06)
#define GB_CAM_JPEG_AVAILABLE_THUMBNAIL_SIZES                        (GB_CAM_JPEG_SECTION | 0x07)
#define GB_CAM_JPEG_MAX_SIZE                                         (GB_CAM_JPEG_SECTION | 0x08)
#define GB_CAM_JPEG_SIZE                                             (GB_CAM_JPEG_SECTION | 0x09)
#define GB_CAM_LENS_APERTURE                                         (GB_CAM_LENS_SECTION | 0x00)
#define GB_CAM_LENS_FILTER_DENSITY                                   (GB_CAM_LENS_SECTION | 0x01)
#define GB_CAM_LENS_FOCAL_LENGTH                                     (GB_CAM_LENS_SECTION | 0x02)
#define GB_CAM_LENS_FOCUS_DISTANCE                                   (GB_CAM_LENS_SECTION | 0x03)
#define GB_CAM_LENS_OPTICAL_STABILIZATION_MODE                       (GB_CAM_LENS_SECTION | 0x04)
#define GB_CAM_LENS_FACING                                           (GB_CAM_LENS_SECTION | 0x05)
#define GB_CAM_LENS_POSE_ROTATION                                    (GB_CAM_LENS_SECTION | 0x06)
#define GB_CAM_LENS_POSE_TRANSLATION                                 (GB_CAM_LENS_SECTION | 0x07)
#define GB_CAM_LENS_FOCUS_RANGE                                      (GB_CAM_LENS_SECTION | 0x08)
#define GB_CAM_LENS_STATE                                            (GB_CAM_LENS_SECTION | 0x09)
#define GB_CAM_LENS_INTRINSIC_CALIBRATION                            (GB_CAM_LENS_SECTION | 0x0a)
#define GB_CAM_LENS_RADIAL_DISTORTION                                (GB_CAM_LENS_SECTION | 0x0b)
#define GB_CAM_LENS_INFO_AVAILABLE_APERTURES                         (GB_CAM_LENS_INFO_SECTION | 0x00)
#define GB_CAM_LENS_INFO_AVAILABLE_FILTER_DENSITIES                  (GB_CAM_LENS_INFO_SECTION | 0x01)
#define GB_CAM_LENS_INFO_AVAILABLE_FOCAL_LENGTHS                     (GB_CAM_LENS_INFO_SECTION | 0x02)
#define GB_CAM_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION             (GB_CAM_LENS_INFO_SECTION | 0x03)
#define GB_CAM_LENS_INFO_HYPERFOCAL_DISTANCE                         (GB_CAM_LENS_INFO_SECTION | 0x04)
#define GB_CAM_LENS_INFO_MINIMUM_FOCUS_DISTANCE                      (GB_CAM_LENS_INFO_SECTION | 0x05)
#define GB_CAM_LENS_INFO_SHADING_MAP_SIZE                            (GB_CAM_LENS_INFO_SECTION | 0x06)
#define GB_CAM_LENS_INFO_FOCUS_DISTANCE_CALIBRATION                  (GB_CAM_LENS_INFO_SECTION | 0x07)
#define GB_CAM_NOISE_REDUCTION_MODE                                  (GB_CAM_NOISE_REDUCTION_SECTION | 0x00)
#define GB_CAM_NOISE_REDUCTION_STRENGTH                              (GB_CAM_NOISE_REDUCTION_SECTION | 0x01)
#define GB_CAM_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES       (GB_CAM_NOISE_REDUCTION_SECTION | 0x02)
#define GB_CAM_QUIRKS_METERING_CROP_REGION                           (GB_CAM_QUIRKS_SECTION | 0x00)
#define GB_CAM_QUIRKS_TRIGGER_AF_WITH_AUTO                           (GB_CAM_QUIRKS_SECTION | 0x01)
#define GB_CAM_QUIRKS_USE_ZSL_FORMAT                                 (GB_CAM_QUIRKS_SECTION | 0x02)
#define GB_CAM_QUIRKS_USE_PARTIAL_RESULT                             (GB_CAM_QUIRKS_SECTION | 0x03)
#define GB_CAM_QUIRKS_PARTIAL_RESULT                                 (GB_CAM_QUIRKS_SECTION | 0x04)
#define GB_CAM_REQUEST_FRAME_COUNT                                   (GB_CAM_REQUEST_SECTION | 0x00)
#define GB_CAM_REQUEST_ID                                            (GB_CAM_REQUEST_SECTION | 0x01)
#define GB_CAM_REQUEST_INPUT_STREAMS                                 (GB_CAM_REQUEST_SECTION | 0x02)
#define GB_CAM_REQUEST_METADATA_MODE                                 (GB_CAM_REQUEST_SECTION | 0x03)
#define GB_CAM_REQUEST_OUTPUT_STREAMS                                (GB_CAM_REQUEST_SECTION | 0x04)
#define GB_CAM_REQUEST_TYPE                                          (GB_CAM_REQUEST_SECTION | 0x05)
#define GB_CAM_REQUEST_MAX_NUM_OUTPUT_STREAMS                        (GB_CAM_REQUEST_SECTION | 0x06)
#define GB_CAM_REQUEST_MAX_NUM_REPROCESS_STREAMS                     (GB_CAM_REQUEST_SECTION | 0x07)
#define GB_CAM_REQUEST_MAX_NUM_INPUT_STREAMS                         (GB_CAM_REQUEST_SECTION | 0x08)
#define GB_CAM_REQUEST_PIPELINE_DEPTH                                (GB_CAM_REQUEST_SECTION | 0x09)
#define GB_CAM_REQUEST_PIPELINE_MAX_DEPTH                            (GB_CAM_REQUEST_SECTION | 0x0a)
#define GB_CAM_REQUEST_PARTIAL_RESULT_COUNT                          (GB_CAM_REQUEST_SECTION | 0x0b)
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES                        (GB_CAM_REQUEST_SECTION | 0x0c)
#define GB_CAM_REQUEST_AVAILABLE_REQUEST_KEYS                        (GB_CAM_REQUEST_SECTION | 0x0d)
#define GB_CAM_REQUEST_AVAILABLE_RESULT_KEYS                         (GB_CAM_REQUEST_SECTION | 0x0e)
#define GB_CAM_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS                (GB_CAM_REQUEST_SECTION | 0x0f)
#define GB_CAM_SCALER_CROP_REGION                                    (GB_CAM_SCALER_SECTION | 0x00)
#define GB_CAM_SCALER_AVAILABLE_FORMATS                              (GB_CAM_SCALER_SECTION | 0x01)
#define GB_CAM_SCALER_AVAILABLE_JPEG_MIN_DURATIONS                   (GB_CAM_SCALER_SECTION | 0x02)
#define GB_CAM_SCALER_AVAILABLE_JPEG_SIZES                           (GB_CAM_SCALER_SECTION | 0x03)
#define GB_CAM_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM                     (GB_CAM_SCALER_SECTION | 0x04)
#define GB_CAM_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS              (GB_CAM_SCALER_SECTION | 0x05)
#define GB_CAM_SCALER_AVAILABLE_PROCESSED_SIZES                      (GB_CAM_SCALER_SECTION | 0x06)
#define GB_CAM_SCALER_AVAILABLE_RAW_MIN_DURATIONS                    (GB_CAM_SCALER_SECTION | 0x07)
#define GB_CAM_SCALER_AVAILABLE_RAW_SIZES                            (GB_CAM_SCALER_SECTION | 0x08)
#define GB_CAM_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP             (GB_CAM_SCALER_SECTION | 0x09)
#define GB_CAM_SCALER_AVAILABLE_STREAM_CONFIGURATIONS                (GB_CAM_SCALER_SECTION | 0x0a)
#define GB_CAM_SCALER_AVAILABLE_MIN_FRAME_DURATIONS                  (GB_CAM_SCALER_SECTION | 0x0b)
#define GB_CAM_SCALER_AVAILABLE_STALL_DURATIONS                      (GB_CAM_SCALER_SECTION | 0x0c)
#define GB_CAM_SCALER_CROPPING_TYPE                                  (GB_CAM_SCALER_SECTION | 0x0d)
#define GB_CAM_SENSOR_EXPOSURE_TIME                                  (GB_CAM_SENSOR_SECTION | 0x00)
#define GB_CAM_SENSOR_FRAME_DURATION                                 (GB_CAM_SENSOR_SECTION | 0x01)
#define GB_CAM_SENSOR_SENSITIVITY                                    (GB_CAM_SENSOR_SECTION | 0x02)
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1                          (GB_CAM_SENSOR_SECTION | 0x03)
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT2                          (GB_CAM_SENSOR_SECTION | 0x04)
#define GB_CAM_SENSOR_CALIBRATION_TRANSFORM1                         (GB_CAM_SENSOR_SECTION | 0x05)
#define GB_CAM_SENSOR_CALIBRATION_TRANSFORM2                         (GB_CAM_SENSOR_SECTION | 0x06)
#define GB_CAM_SENSOR_COLOR_TRANSFORM1                               (GB_CAM_SENSOR_SECTION | 0x07)
#define GB_CAM_SENSOR_COLOR_TRANSFORM2                               (GB_CAM_SENSOR_SECTION | 0x08)
#define GB_CAM_SENSOR_FORWARD_MATRIX1                                (GB_CAM_SENSOR_SECTION | 0x09)
#define GB_CAM_SENSOR_FORWARD_MATRIX2                                (GB_CAM_SENSOR_SECTION | 0x0a)
#define GB_CAM_SENSOR_BASE_GAIN_FACTOR                               (GB_CAM_SENSOR_SECTION | 0x0b)
#define GB_CAM_SENSOR_BLACK_LEVEL_PATTERN                            (GB_CAM_SENSOR_SECTION | 0x0c)
#define GB_CAM_SENSOR_MAX_ANALOG_SENSITIVITY                         (GB_CAM_SENSOR_SECTION | 0x0d)
#define GB_CAM_SENSOR_ORIENTATION                                    (GB_CAM_SENSOR_SECTION | 0x0e)
#define GB_CAM_SENSOR_PROFILE_HUE_SAT_MAP_DIMENSIONS                 (GB_CAM_SENSOR_SECTION | 0x0f)
#define GB_CAM_SENSOR_TIMESTAMP                                      (GB_CAM_SENSOR_SECTION | 0x10)
#define GB_CAM_SENSOR_TEMPERATURE                                    (GB_CAM_SENSOR_SECTION | 0x11)
#define GB_CAM_SENSOR_NEUTRAL_COLOR_POINT                            (GB_CAM_SENSOR_SECTION | 0x12)
#define GB_CAM_SENSOR_NOISE_PROFILE                                  (GB_CAM_SENSOR_SECTION | 0x13)
#define GB_CAM_SENSOR_PROFILE_HUE_SAT_MAP                            (GB_CAM_SENSOR_SECTION | 0x14)
#define GB_CAM_SENSOR_PROFILE_TONE_CURVE                             (GB_CAM_SENSOR_SECTION | 0x15)
#define GB_CAM_SENSOR_GREEN_SPLIT                                    (GB_CAM_SENSOR_SECTION | 0x16)
#define GB_CAM_SENSOR_TEST_PATTERN_DATA                              (GB_CAM_SENSOR_SECTION | 0x17)
#define GB_CAM_SENSOR_TEST_PATTERN_MODE                              (GB_CAM_SENSOR_SECTION | 0x18)
#define GB_CAM_SENSOR_AVAILABLE_TEST_PATTERN_MODES                   (GB_CAM_SENSOR_SECTION | 0x19)
#define GB_CAM_SENSOR_ROLLING_SHUTTER_SKEW                           (GB_CAM_SENSOR_SECTION | 0x1a)
#define GB_CAM_SENSOR_INFO_ACTIVE_ARRAY_SIZE                         (GB_CAM_SENSOR_INFO_SECTION | 0x00)
#define GB_CAM_SENSOR_INFO_SENSITIVITY_RANGE                         (GB_CAM_SENSOR_INFO_SECTION | 0x01)
#define GB_CAM_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT                  (GB_CAM_SENSOR_INFO_SECTION | 0x02)
#define GB_CAM_SENSOR_INFO_EXPOSURE_TIME_RANGE                       (GB_CAM_SENSOR_INFO_SECTION | 0x03)
#define GB_CAM_SENSOR_INFO_MAX_FRAME_DURATION                        (GB_CAM_SENSOR_INFO_SECTION | 0x04)
#define GB_CAM_SENSOR_INFO_PHYSICAL_SIZE                             (GB_CAM_SENSOR_INFO_SECTION | 0x05)
#define GB_CAM_SENSOR_INFO_PIXEL_ARRAY_SIZE                          (GB_CAM_SENSOR_INFO_SECTION | 0x06)
#define GB_CAM_SENSOR_INFO_WHITE_LEVEL                               (GB_CAM_SENSOR_INFO_SECTION | 0x07)
#define GB_CAM_SENSOR_INFO_TIMESTAMP_SOURCE                          (GB_CAM_SENSOR_INFO_SECTION | 0x08)
#define GB_CAM_SENSOR_INFO_LENS_SHADING_APPLIED                      (GB_CAM_SENSOR_INFO_SECTION | 0x09)
#define GB_CAM_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE          (GB_CAM_SENSOR_INFO_SECTION | 0x0a)
#define GB_CAM_SHADING_MODE                                          (GB_CAM_SHADING_SECTION | 0x00)
#define GB_CAM_SHADING_STRENGTH                                      (GB_CAM_SHADING_SECTION | 0x01)
#define GB_CAM_SHADING_AVAILABLE_MODES                               (GB_CAM_SHADING_SECTION | 0x02)
#define GB_CAM_STATISTICS_FACE_DETECT_MODE                           (GB_CAM_STATISTICS_SECTION | 0x00)
#define GB_CAM_STATISTICS_HISTOGRAM_MODE                             (GB_CAM_STATISTICS_SECTION | 0x01)
#define GB_CAM_STATISTICS_SHARPNESS_MAP_MODE                         (GB_CAM_STATISTICS_SECTION | 0x02)
#define GB_CAM_STATISTICS_HOT_PIXEL_MAP_MODE                         (GB_CAM_STATISTICS_SECTION | 0x03)
#define GB_CAM_STATISTICS_FACE_IDS                                   (GB_CAM_STATISTICS_SECTION | 0x04)
#define GB_CAM_STATISTICS_FACE_LANDMARKS                             (GB_CAM_STATISTICS_SECTION | 0x05)
#define GB_CAM_STATISTICS_FACE_RECTANGLES                            (GB_CAM_STATISTICS_SECTION | 0x06)
#define GB_CAM_STATISTICS_FACE_SCORES                                (GB_CAM_STATISTICS_SECTION | 0x07)
#define GB_CAM_STATISTICS_HISTOGRAM                                  (GB_CAM_STATISTICS_SECTION | 0x08)
#define GB_CAM_STATISTICS_SHARPNESS_MAP                              (GB_CAM_STATISTICS_SECTION | 0x09)
#define GB_CAM_STATISTICS_LENS_SHADING_CORRECTION_MAP                (GB_CAM_STATISTICS_SECTION | 0x0a)
#define GB_CAM_STATISTICS_LENS_SHADING_MAP                           (GB_CAM_STATISTICS_SECTION | 0x0b)
#define GB_CAM_STATISTICS_PREDICTED_COLOR_GAINS                      (GB_CAM_STATISTICS_SECTION | 0x0c)
#define GB_CAM_STATISTICS_PREDICTED_COLOR_TRANSFORM                  (GB_CAM_STATISTICS_SECTION | 0x0d)
#define GB_CAM_STATISTICS_SCENE_FLICKER                              (GB_CAM_STATISTICS_SECTION | 0x0e)
#define GB_CAM_STATISTICS_HOT_PIXEL_MAP                              (GB_CAM_STATISTICS_SECTION | 0x0f)
#define GB_CAM_STATISTICS_LENS_SHADING_MAP_MODE                      (GB_CAM_STATISTICS_SECTION | 0x10)
#define GB_CAM_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES           (GB_CAM_STATISTICS_INFO_SECTION | 0x00)
#define GB_CAM_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT                (GB_CAM_STATISTICS_INFO_SECTION | 0x01)
#define GB_CAM_STATISTICS_INFO_MAX_FACE_COUNT                        (GB_CAM_STATISTICS_INFO_SECTION | 0x02)
#define GB_CAM_STATISTICS_INFO_MAX_HISTOGRAM_COUNT                   (GB_CAM_STATISTICS_INFO_SECTION | 0x03)
#define GB_CAM_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE               (GB_CAM_STATISTICS_INFO_SECTION | 0x04)
#define GB_CAM_STATISTICS_INFO_SHARPNESS_MAP_SIZE                    (GB_CAM_STATISTICS_INFO_SECTION | 0x05)
#define GB_CAM_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES         (GB_CAM_STATISTICS_INFO_SECTION | 0x06)
#define GB_CAM_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES      (GB_CAM_STATISTICS_INFO_SECTION | 0x07)
#define GB_CAM_TONEMAP_CURVE_BLUE                                    (GB_CAM_TONEMAP_SECTION | 0x00)
#define GB_CAM_TONEMAP_CURVE_GREEN                                   (GB_CAM_TONEMAP_SECTION | 0x01)
#define GB_CAM_TONEMAP_CURVE_RED                                     (GB_CAM_TONEMAP_SECTION | 0x02)
#define GB_CAM_TONEMAP_MODE                                          (GB_CAM_TONEMAP_SECTION | 0x03)
#define GB_CAM_TONEMAP_MAX_CURVE_POINTS                              (GB_CAM_TONEMAP_SECTION | 0x04)
#define GB_CAM_TONEMAP_AVAILABLE_TONE_MAP_MODES                      (GB_CAM_TONEMAP_SECTION | 0x05)
#define GB_CAM_TONEMAP_GAMMA                                         (GB_CAM_TONEMAP_SECTION | 0x06)
#define GB_CAM_TONEMAP_PRESET_CURVE                                  (GB_CAM_TONEMAP_SECTION | 0x07)
#define GB_CAM_LED_TRANSMIT                                          (GB_CAM_LED_SECTION | 0x00)
#define GB_CAM_LED_AVAILABLE_LEDS                                    (GB_CAM_LED_SECTION | 0x01)
#define GB_CAM_INFO_SUPPORTED_HARDWARE_LEVEL                         (GB_CAM_INFO_SECTION | 0x00)
#define GB_CAM_BLACK_LEVEL_LOCK                                      (GB_CAM_BLACK_LEVEL_SECTION | 0x00)
#define GB_CAM_SYNC_FRAME_NUMBER                                     (GB_CAM_SYNC_SECTION | 0x00)
#define GB_CAM_SYNC_MAX_LATENCY                                      (GB_CAM_SYNC_SECTION | 0x01)
#define GB_CAM_REPROCESS_EFFECTIVE_EXPOSURE_FACTOR                   (GB_CAM_REPROCESS_SECTION | 0x00)
#define GB_CAM_REPROCESS_MAX_CAPTURE_STALL                           (GB_CAM_REPROCESS_SECTION | 0x01)
#define GB_CAM_DEPTH_MAX_DEPTH_SAMPLES                               (GB_CAM_DEPTH_SECTION | 0x00)
#define GB_CAM_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS           (GB_CAM_DEPTH_SECTION | 0x01)
#define GB_CAM_DEPTH_AVAILABLE_DEPTH_MIN_FRAME_DURATIONS             (GB_CAM_DEPTH_SECTION | 0x02)
#define GB_CAM_DEPTH_AVAILABLE_DEPTH_STALL_DURATIONS                 (GB_CAM_DEPTH_SECTION | 0x03)
#define GB_CAM_DEPTH_DEPTH_IS_EXCLUSIVE                              (GB_CAM_DEPTH_SECTION | 0x04)
#define GB_CAM_ARA_FEATURE_JPEG                                      (GB_CAM_ARA_SECTION | 0x00)
#define GB_CAM_ARA_FEATURE_SCALER                                    (GB_CAM_ARA_SECTION | 0x01)
#define GB_CAM_ARA_METADATA_FORMAT                                   (GB_CAM_ARA_SECTION | 0x02)
#define GB_CAM_ARA_METADATA_TRANSPORT                                (GB_CAM_ARA_SECTION | 0x03)

/**
 * Enumeration definitions for the various entries that need them
 */
/* GB_CAM_COLOR_CORRECTION_MODE */
#define GB_CAM_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX                0
#define GB_CAM_COLOR_CORRECTION_MODE_FAST                            1
#define GB_CAM_COLOR_CORRECTION_MODE_HIGH_QUALITY                    2

/* GB_CAM_COLOR_CORRECTION_ABERRATION_MODE */
#define GB_CAM_COLOR_CORRECTION_ABERRATION_MODE_OFF                  0
#define GB_CAM_COLOR_CORRECTION_ABERRATION_MODE_FAST                 1
#define GB_CAM_COLOR_CORRECTION_ABERRATION_MODE_HIGH_QUALITY         2

/* GB_CAM_CONTROL_AE_ANTIBANDING_MODE */
#define GB_CAM_CONTROL_AE_ANTIBANDING_MODE_OFF                       0
#define GB_CAM_CONTROL_AE_ANTIBANDING_MODE_50HZ                      1
#define GB_CAM_CONTROL_AE_ANTIBANDING_MODE_60HZ                      2
#define GB_CAM_CONTROL_AE_ANTIBANDING_MODE_AUTO                      3

/* GB_CAM_CONTROL_AE_LOCK */
#define GB_CAM_CONTROL_AE_LOCK_OFF                                   0
#define GB_CAM_CONTROL_AE_LOCK_ON                                    1

/* GB_CAM_CONTROL_AE_MODE */
#define GB_CAM_CONTROL_AE_MODE_OFF                                   0
#define GB_CAM_CONTROL_AE_MODE_ON                                    1
#define GB_CAM_CONTROL_AE_MODE_ON_AUTO_FLASH                         2
#define GB_CAM_CONTROL_AE_MODE_ON_ALWAYS_FLASH                       3
#define GB_CAM_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE                  4

/* GB_CAM_CONTROL_AE_PRECAPTURE_TRIGGER */
#define GB_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE                    0
#define GB_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_START                   1
#define GB_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL                  2

/* GB_CAM_CONTROL_AF_MODE */
#define GB_CAM_CONTROL_AF_MODE_OFF                                   0
#define GB_CAM_CONTROL_AF_MODE_AUTO                                  1
#define GB_CAM_CONTROL_AF_MODE_MACRO                                 2
#define GB_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO                      3
#define GB_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE                    4
#define GB_CAM_CONTROL_AF_MODE_EDOF                                  5

/* GB_CAM_CONTROL_AF_TRIGGER */
#define GB_CAM_CONTROL_AF_TRIGGER_IDLE                               0
#define GB_CAM_CONTROL_AF_TRIGGER_START                              1
#define GB_CAM_CONTROL_AF_TRIGGER_CANCEL                             2

/* GB_CAM_CONTROL_AWB_LOCK */
#define GB_CAM_CONTROL_AWB_LOCK_OFF                                  0
#define GB_CAM_CONTROL_AWB_LOCK_ON                                   1

/* GB_CAM_CONTROL_AWB_MODE */
#define GB_CAM_CONTROL_AWB_MODE_OFF                                  0
#define GB_CAM_CONTROL_AWB_MODE_AUTO                                 1
#define GB_CAM_CONTROL_AWB_MODE_INCANDESCENT                         2
#define GB_CAM_CONTROL_AWB_MODE_FLUORESCENT                          3
#define GB_CAM_CONTROL_AWB_MODE_WARM_FLUORESCENT                     4
#define GB_CAM_CONTROL_AWB_MODE_DAYLIGHT                             5
#define GB_CAM_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT                      6
#define GB_CAM_CONTROL_AWB_MODE_TWILIGHT                             7
#define GB_CAM_CONTROL_AWB_MODE_SHADE                                8

/* GB_CAM_CONTROL_CAPTURE_INTENT */
#define GB_CAM_CONTROL_CAPTURE_INTENT_CUSTOM                         0
#define GB_CAM_CONTROL_CAPTURE_INTENT_PREVIEW                        1
#define GB_CAM_CONTROL_CAPTURE_INTENT_STILL_CAPTURE                  2
#define GB_CAM_CONTROL_CAPTURE_INTENT_VIDEO_RECORD                   3
#define GB_CAM_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT                 4
#define GB_CAM_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG               5
#define GB_CAM_CONTROL_CAPTURE_INTENT_MANUAL                         6

/* GB_CAM_CONTROL_EFFECT_MODE */
#define GB_CAM_CONTROL_EFFECT_MODE_OFF                               0
#define GB_CAM_CONTROL_EFFECT_MODE_MONO                              1
#define GB_CAM_CONTROL_EFFECT_MODE_NEGATIVE                          2
#define GB_CAM_CONTROL_EFFECT_MODE_SOLARIZE                          3
#define GB_CAM_CONTROL_EFFECT_MODE_SEPIA                             4
#define GB_CAM_CONTROL_EFFECT_MODE_POSTERIZE                         5
#define GB_CAM_CONTROL_EFFECT_MODE_WHITEBOARD                        6
#define GB_CAM_CONTROL_EFFECT_MODE_BLACKBOARD                        7
#define GB_CAM_CONTROL_EFFECT_MODE_AQUA                              8

/* GB_CAM_CONTROL_MODE */
#define GB_CAM_CONTROL_MODE_OFF                                      0
#define GB_CAM_CONTROL_MODE_AUTO                                     1
#define GB_CAM_CONTROL_MODE_USE_SCENE_MODE                           2
#define GB_CAM_CONTROL_MODE_OFF_KEEP_STATE                           3

/* GB_CAM_CONTROL_SCENE_MODE */
#define GB_CAM_CONTROL_SCENE_MODE_DISABLED                           0
#define GB_CAM_CONTROL_SCENE_MODE_FACE_PRIORITY                      0
#define GB_CAM_CONTROL_SCENE_MODE_ACTION                             1
#define GB_CAM_CONTROL_SCENE_MODE_PORTRAIT                           2
#define GB_CAM_CONTROL_SCENE_MODE_LANDSCAPE                          3
#define GB_CAM_CONTROL_SCENE_MODE_NIGHT                              4
#define GB_CAM_CONTROL_SCENE_MODE_NIGHT_PORTRAIT                     5
#define GB_CAM_CONTROL_SCENE_MODE_THEATRE                            6
#define GB_CAM_CONTROL_SCENE_MODE_BEACH                              7
#define GB_CAM_CONTROL_SCENE_MODE_SNOW                               8
#define GB_CAM_CONTROL_SCENE_MODE_SUNSET                             9
#define GB_CAM_CONTROL_SCENE_MODE_STEADYPHOTO                        10
#define GB_CAM_CONTROL_SCENE_MODE_FIREWORKS                          11
#define GB_CAM_CONTROL_SCENE_MODE_SPORTS                             12
#define GB_CAM_CONTROL_SCENE_MODE_PARTY                              13
#define GB_CAM_CONTROL_SCENE_MODE_CANDLELIGHT                        14
#define GB_CAM_CONTROL_SCENE_MODE_BARCODE                            15
#define GB_CAM_CONTROL_SCENE_MODE_HIGH_SPEED_VIDEO                   16
#define GB_CAM_CONTROL_SCENE_MODE_HDR                                17
#define GB_CAM_CONTROL_SCENE_MODE_FACE_PRIORITY_LOW_LIGHT            18

/* GB_CAM_CONTROL_VIDEO_STABILIZATION_MODE */
#define GB_CAM_CONTROL_VIDEO_STABILIZATION_MODE_OFF                  0
#define GB_CAM_CONTROL_VIDEO_STABILIZATION_MODE_ON                   1

/* GB_CAM_CONTROL_AE_STATE */
#define GB_CAM_CONTROL_AE_STATE_INACTIVE                             0
#define GB_CAM_CONTROL_AE_STATE_SEARCHING                            1
#define GB_CAM_CONTROL_AE_STATE_CONVERGED                            2
#define GB_CAM_CONTROL_AE_STATE_LOCKED                               3
#define GB_CAM_CONTROL_AE_STATE_FLASH_REQUIRED                       4
#define GB_CAM_CONTROL_AE_STATE_PRECAPTURE                           5

/* GB_CAM_CONTROL_AF_STATE */
#define GB_CAM_CONTROL_AF_STATE_INACTIVE                             0
#define GB_CAM_CONTROL_AF_STATE_PASSIVE_SCAN                         1
#define GB_CAM_CONTROL_AF_STATE_PASSIVE_FOCUSED                      2
#define GB_CAM_CONTROL_AF_STATE_ACTIVE_SCAN                          3
#define GB_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED                       4
#define GB_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED                   5
#define GB_CAM_CONTROL_AF_STATE_PASSIVE_UNFOCUSED                    6

/* GB_CAM_CONTROL_AWB_STATE */
#define GB_CAM_CONTROL_AWB_STATE_INACTIVE                            0
#define GB_CAM_CONTROL_AWB_STATE_SEARCHING                           1
#define GB_CAM_CONTROL_AWB_STATE_CONVERGED                           2
#define GB_CAM_CONTROL_AWB_STATE_LOCKED                              3

/* GB_CAM_CONTROL_AE_LOCK_AVAILABLE */
#define GB_CAM_CONTROL_AE_LOCK_AVAILABLE_FALSE                       0
#define GB_CAM_CONTROL_AE_LOCK_AVAILABLE_TRUE                        1

/* GB_CAM_CONTROL_AWB_LOCK_AVAILABLE */
#define GB_CAM_CONTROL_AWB_LOCK_AVAILABLE_FALSE                      0
#define GB_CAM_CONTROL_AWB_LOCK_AVAILABLE_TRUE                       1

/* GB_CAM_DEMOSAIC_MODE */
#define GB_CAM_DEMOSAIC_MODE_FAST                                    0
#define GB_CAM_DEMOSAIC_MODE_HIGH_QUALITY                            1

/* GB_CAM_EDGE_MODE */
#define GB_CAM_EDGE_MODE_OFF                                         0
#define GB_CAM_EDGE_MODE_FAST                                        1
#define GB_CAM_EDGE_MODE_HIGH_QUALITY                                2
#define GB_CAM_EDGE_MODE_ZERO_SHUTTER_LAG                            3

/* GB_CAM_FLASH_MODE */
#define GB_CAM_FLASH_MODE_OFF                                        0
#define GB_CAM_FLASH_MODE_SINGLE                                     1
#define GB_CAM_FLASH_MODE_TORCH                                      2

/* GB_CAM_FLASH_STATE */
#define GB_CAM_FLASH_STATE_UNAVAILABLE                               0
#define GB_CAM_FLASH_STATE_CHARGING                                  1
#define GB_CAM_FLASH_STATE_READY                                     2
#define GB_CAM_FLASH_STATE_FIRED                                     3
#define GB_CAM_FLASH_STATE_PARTIAL                                   4

/* GB_CAM_FLASH_INFO_AVAILABLE */
#define GB_CAM_FLASH_INFO_AVAILABLE_FALSE                            0
#define GB_CAM_FLASH_INFO_AVAILABLE_TRUE                             1

/* GB_CAM_HOT_PIXEL_MODE */
#define GB_CAM_HOT_PIXEL_MODE_OFF                                    0
#define GB_CAM_HOT_PIXEL_MODE_FAST                                   1
#define GB_CAM_HOT_PIXEL_MODE_HIGH_QUALITY                           2

/* GB_CAM_LENS_OPTICAL_STABILIZATION_MODE */
#define GB_CAM_LENS_OPTICAL_STABILIZATION_MODE_OFF                   0
#define GB_CAM_LENS_OPTICAL_STABILIZATION_MODE_ON                    1

/* GB_CAM_LENS_FACING */
#define GB_CAM_LENS_FACING_FRONT                                     0
#define GB_CAM_LENS_FACING_BACK                                      1
#define GB_CAM_LENS_FACING_EXTERNAL                                  2

/* GB_CAM_LENS_STATE */
#define GB_CAM_LENS_STATE_STATIONARY                                 0
#define GB_CAM_LENS_STATE_MOVING                                     1

/* GB_CAM_LENS_INFO_FOCUS_DISTANCE_CALIBRATION */
#define GB_CAM_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED     0
#define GB_CAM_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_APPROXIMATE      1
#define GB_CAM_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED       2

/* GB_CAM_NOISE_REDUCTION_MODE */
#define GB_CAM_NOISE_REDUCTION_MODE_OFF                              0
#define GB_CAM_NOISE_REDUCTION_MODE_FAST                             1
#define GB_CAM_NOISE_REDUCTION_MODE_HIGH_QUALITY                     2
#define GB_CAM_NOISE_REDUCTION_MODE_MINIMAL                          3
#define GB_CAM_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG                 4

/* GB_CAM_QUIRKS_PARTIAL_RESULT */
#define GB_CAM_QUIRKS_PARTIAL_RESULT_FINAL                           0
#define GB_CAM_QUIRKS_PARTIAL_RESULT_PARTIAL                         1

/* GB_CAM_REQUEST_METADATA_MODE */
#define GB_CAM_REQUEST_METADATA_MODE_NONE                            0
#define GB_CAM_REQUEST_METADATA_MODE_FULL                            1

/* GB_CAM_REQUEST_TYPE */
#define GB_CAM_REQUEST_TYPE_CAPTURE                                  0
#define GB_CAM_REQUEST_TYPE_REPROCESS                                1

/* GB_CAM_REQUEST_AVAILABLE_CAPABILITIES */
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE    0
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR          1
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING 2
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_RAW                    3
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING   4
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_READ_SENSOR_SETTINGS   5
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE          6
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING       7
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_DEPTH_OUTPUT           8
#define GB_CAM_REQUEST_AVAILABLE_CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO 9

/* GB_CAM_SCALER_AVAILABLE_FORMATS */
#define GB_CAM_SCALER_AVAILABLE_FORMATS_RAW16                        0x20
#define GB_CAM_SCALER_AVAILABLE_FORMATS_RAW_OPAQUE                   0x24
#define GB_CAM_SCALER_AVAILABLE_FORMATS_YV12                         0x32315659
#define GB_CAM_SCALER_AVAILABLE_FORMATS_YCrCb_420_SP                 0x11
#define GB_CAM_SCALER_AVAILABLE_FORMATS_IMPLEMENTATION_DEFINED       0x22
#define GB_CAM_SCALER_AVAILABLE_FORMATS_YCbCr_420_888                0x23
#define GB_CAM_SCALER_AVAILABLE_FORMATS_BLOB                         0x21

/* GB_CAM_SCALER_AVAILABLE_STREAM_CONFIGURATIONS */
#define GB_CAM_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT         0
#define GB_CAM_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT          1

/* GB_CAM_SCALER_CROPPING_TYPE */
#define GB_CAM_SCALER_CROPPING_TYPE_CENTER_ONLY                      0
#define GB_CAM_SCALER_CROPPING_TYPE_FREEFORM                         1

/* GB_CAM_SENSOR_REFERENCE_ILLUMINANT1 */
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT                 1
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_FLUORESCENT              2
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_TUNGSTEN                 3
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_FLASH                    4
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_FINE_WEATHER             9
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_CLOUDY_WEATHER           10
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_SHADE                    11
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT_FLUORESCENT     12
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_DAY_WHITE_FLUORESCENT    13
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_COOL_WHITE_FLUORESCENT   14
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_WHITE_FLUORESCENT        15
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A               17
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_B               18
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_C               19
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_D55                      20
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_D65                      21
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_D75                      22
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_D50                      23
#define GB_CAM_SENSOR_REFERENCE_ILLUMINANT1_ISO_STUDIO_TUNGSTEN      24

/* GB_CAM_SENSOR_TEST_PATTERN_MODE */
#define GB_CAM_SENSOR_TEST_PATTERN_MODE_OFF                          0
#define GB_CAM_SENSOR_TEST_PATTERN_MODE_SOLID_COLOR                  1
#define GB_CAM_SENSOR_TEST_PATTERN_MODE_COLOR_BARS                   2
#define GB_CAM_SENSOR_TEST_PATTERN_MODE_COLOR_BARS_FADE_TO_GRAY      3
#define GB_CAM_SENSOR_TEST_PATTERN_MODE_PN9                          4
#define GB_CAM_SENSOR_TEST_PATTERN_MODE_CUSTOM1                      256

/* GB_CAM_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT */
#define GB_CAM_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB             0
#define GB_CAM_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG             1
#define GB_CAM_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG             2
#define GB_CAM_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR             3
#define GB_CAM_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGB              4

/* GB_CAM_SENSOR_INFO_TIMESTAMP_SOURCE */
#define GB_CAM_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN                  0
#define GB_CAM_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME                 1

/* GB_CAM_SENSOR_INFO_LENS_SHADING_APPLIED */
#define GB_CAM_SENSOR_INFO_LENS_SHADING_APPLIED_FALSE                0
#define GB_CAM_SENSOR_INFO_LENS_SHADING_APPLIED_TRUE                 1

/* GB_CAM_SHADING_MODE */
#define GB_CAM_SHADING_MODE_OFF                                      0
#define GB_CAM_SHADING_MODE_FAST                                     1
#define GB_CAM_SHADING_MODE_HIGH_QUALITY                             2

/* GB_CAM_STATISTICS_FACE_DETECT_MODE */
#define GB_CAM_STATISTICS_FACE_DETECT_MODE_OFF                       0
#define GB_CAM_STATISTICS_FACE_DETECT_MODE_SIMPLE                    1
#define GB_CAM_STATISTICS_FACE_DETECT_MODE_FULL                      2

/* GB_CAM_STATISTICS_HISTOGRAM_MODE */
#define GB_CAM_STATISTICS_HISTOGRAM_MODE_OFF                         0
#define GB_CAM_STATISTICS_HISTOGRAM_MODE_ON                          1

/* GB_CAM_STATISTICS_SHARPNESS_MAP_MODE */
#define GB_CAM_STATISTICS_SHARPNESS_MAP_MODE_OFF                     0
#define GB_CAM_STATISTICS_SHARPNESS_MAP_MODE_ON                      1

/* GB_CAM_STATISTICS_HOT_PIXEL_MAP_MODE */
#define GB_CAM_STATISTICS_HOT_PIXEL_MAP_MODE_OFF                     0
#define GB_CAM_STATISTICS_HOT_PIXEL_MAP_MODE_ON                      1

/* GB_CAM_STATISTICS_SCENE_FLICKER */
#define GB_CAM_STATISTICS_SCENE_FLICKER_NONE                         0
#define GB_CAM_STATISTICS_SCENE_FLICKER_50HZ                         1
#define GB_CAM_STATISTICS_SCENE_FLICKER_60HZ                         2

/* GB_CAM_STATISTICS_LENS_SHADING_MAP_MODE */
#define GB_CAM_STATISTICS_LENS_SHADING_MAP_MODE_OFF                  0
#define GB_CAM_STATISTICS_LENS_SHADING_MAP_MODE_ON                   1

/* GB_CAM_TONEMAP_MODE */
#define GB_CAM_TONEMAP_MODE_CONTRAST_CURVE                           0
#define GB_CAM_TONEMAP_MODE_FAST                                     1
#define GB_CAM_TONEMAP_MODE_HIGH_QUALITY                             2
#define GB_CAM_TONEMAP_MODE_GAMMA_VALUE                              3
#define GB_CAM_TONEMAP_MODE_PRESET_CURVE                             4

/* GB_CAM_TONEMAP_PRESET_CURVE */
#define GB_CAM_TONEMAP_PRESET_CURVE_SRGB                             0
#define GB_CAM_TONEMAP_PRESET_CURVE_REC709                           1

/* GB_CAM_LED_TRANSMIT */
#define GB_CAM_LED_TRANSMIT_OFF                                      0
#define GB_CAM_LED_TRANSMIT_ON                                       1

/* GB_CAM_LED_AVAILABLE_LEDS */
#define GB_CAM_LED_AVAILABLE_LEDS_TRANSMIT                           0

/* GB_CAM_INFO_SUPPORTED_HARDWARE_LEVEL */
#define GB_CAM_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED                 0
#define GB_CAM_INFO_SUPPORTED_HARDWARE_LEVEL_FULL                    1
#define GB_CAM_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY                  2

/* GB_CAM_BLACK_LEVEL_LOCK */
#define GB_CAM_BLACK_LEVEL_LOCK_OFF                                  0
#define GB_CAM_BLACK_LEVEL_LOCK_ON                                   1

/* GB_CAM_SYNC_FRAME_NUMBER */
#define GB_CAM_SYNC_FRAME_NUMBER_CONVERGING                          -1
#define GB_CAM_SYNC_FRAME_NUMBER_UNKNOWN                             -2

/* GB_CAM_SYNC_MAX_LATENCY */
#define GB_CAM_SYNC_MAX_LATENCY_PER_FRAME_CONTROL                    0
#define GB_CAM_SYNC_MAX_LATENCY_UNKNOWN                              -1

/* GB_CAM_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS */
#define GB_CAM_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS_OUTPUT    0
#define GB_CAM_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS_INPUT     1

/* GB_CAM_DEPTH_DEPTH_IS_EXCLUSIVE */
#define GB_CAM_DEPTH_DEPTH_IS_EXCLUSIVE_FALSE                        0
#define GB_CAM_DEPTH_DEPTH_IS_EXCLUSIVE_TRUE                         1

/* GB_CAM_PROPERTY_ARA_SUPPORTED_JPEG */
#define GB_CAM_ARA_FEATURE_JPEG_FALSE                                0
#define GB_CAM_ARA_FEATURE_JPEG_TRUE                                 1

/* GB_CAM_PROPERTY_ARA_SUPPORTED_SCALER */
#define GB_CAM_ARA_FEATURE_SCALER_FALSE                              0
#define GB_CAM_ARA_FEATURE_SCALER_TRUE                               1

/* GB_CAM_PROPERTY_ARA_METADATA_FORMAT */
#define GB_CAM_ARA_METADATA_FORMAT_ARA                               0
#define GB_CAM_ARA_METADATA_FORMAT_CUSTOM                            1

/* GB_CAM_PROPERTY_ARA_METADATA_TRANSPORT */
#define GB_CAM_ARA_METADATA_TRANSPORT_NONE                           0
#define GB_CAM_ARA_METADATA_TRANSPORT_CSI                            1
#define GB_CAM_ARA_METADATA_TRANSPORT_GREYBUS                        2

/** A single camera property */
struct gb_camera_property {
    /** The property id */
    uint16_t key;
    /** The size of the property's data (padding excluded) */
    uint16_t len;
    /** The property data */
    uint8_t data[0];
};
#define GB_CAM_PROPERTY_PACKET_HEADER_SIZE  4

/** A camera property packet */
struct gb_camera_property_packet {
    /** Total size of the property packet */
    uint16_t size;
    /** Number of properties in this packet */
    uint16_t nprops;
    /** The array of properties */
    struct gb_camera_property props[0];
};
#define GB_CAM_PROPERTY_HEADER_SIZE         4

#define GB_CAM_PROPERTY_ALIGNEMENT          4
#define GB_CAM_ALIGN_PROPERTY(size)\
    (((size) + GB_CAM_PROPERTY_ALIGNEMENT - 1) & ~(GB_CAM_PROPERTY_ALIGNEMENT - 1))

#endif /* __INCLUDE_NUTTX_CAMERA_PROPERTIES__ */
