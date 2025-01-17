/*
 * Copyright (c) 2010 Yamaha Corporation
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef __YAS_CFG_H__
#define __YAS_CFG_H__

#define YAS_MAG_DRIVER_NONE                 (-1)
#define YAS_MAG_DRIVER_YAS529               (1)
#define YAS_MAG_DRIVER_YAS530               (2)

#define YAS_ACC_DRIVER_NONE                 (-1)
#define YAS_ACC_DRIVER_ADXL345              (0)
#define YAS_ACC_DRIVER_ADXL346              (1)
#define YAS_ACC_DRIVER_BMA150               (2)
#define YAS_ACC_DRIVER_BMA222               (3)
#define YAS_ACC_DRIVER_BMA250               (4)
#define YAS_ACC_DRIVER_KXSD9                (5)
#define YAS_ACC_DRIVER_KXTE9                (6)
#define YAS_ACC_DRIVER_KXTF9                (7)
#define YAS_ACC_DRIVER_KXUD9                (8)
#define YAS_ACC_DRIVER_LIS331DL             (9)
#define YAS_ACC_DRIVER_LIS331DLH            (10)
#define YAS_ACC_DRIVER_LIS331DLM            (11)
#define YAS_ACC_DRIVER_LIS3DH               (12)
#define YAS_ACC_DRIVER_MMA8452Q             (13)
#define YAS_ACC_DRIVER_MMA8453Q             (14)

#define YAS_GYRO_DRIVER_NONE                (-1)
#define YAS_GYRO_DRIVER_ITG3200             (0)
#define YAS_GYRO_DRIVER_L3G4200D            (1)

/*----------------------------------------------------------------------------*/
/*                               Configuration                                */
/*----------------------------------------------------------------------------*/

#define YAS_ACC_DRIVER                      (YAS_ACC_DRIVER_LIS3DH)

#if !defined(CONFIG_MACH_AEGIS) && !defined(CONFIG_MACH_VIPER) \
	&& !defined(CONFIG_TIKAL_USCC)
#if defined(CONFIG_INPUT_YAS530) 
#define YAS_MAG_DRIVER						(YAS_MAG_DRIVER_YAS530)
#else
#define YAS_MAG_DRIVER						(YAS_MAG_DRIVER_YAS529)
#endif
#endif

#define YAS_GYRO_DRIVER                     (YAS_GYRO_DRIVER_NONE)

/*----------------------------------------------------------------------------*/
/*                   Acceleration Calibration Configuration                   */
/*----------------------------------------------------------------------------*/

#define YAS_DEFAULT_ACCCALIB_LENGTH         (20)

#if YAS_ACC_DRIVER == YAS_ACC_DRIVER_ADXL345
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (8000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_ADXL346
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (4000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_BMA150
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (4000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_BMA222
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (25000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_BMA250
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (20000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_KXSD9
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (80000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_KXTE9
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (400000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_KXTF9
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (2000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_KXUD9
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (20000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_LIS331DL
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (17000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_LIS331DLH
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (6000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_LIS331DLM
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (28000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_LIS3DH
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (18000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_MMA8452Q
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (1000)
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_MMA8453Q
#define YAS_DEFAULT_ACCCALIB_DISTORTION     (1000)
#else
#error "unknown accelerometer"
#endif


/*----------------------------------------------------------------------------*/
/*                     Accelerometer Filter Configuration                     */
/*----------------------------------------------------------------------------*/
#if YAS_ACC_DRIVER == YAS_ACC_DRIVER_ADXL345
#define YAS_ACC_DEFAULT_FILTER_THRESH       (76612)  /* ((38,306 um/s^2)/count) * 2  */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_ADXL346
#define YAS_ACC_DEFAULT_FILTER_THRESH       (76612)  /* ((38,306 um/s^2)/count) * 2  */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_BMA150
#define YAS_ACC_DEFAULT_FILTER_THRESH       (76612)  /* ((38,306 um/s^2)/count) * 2  */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_BMA222
#define YAS_ACC_DEFAULT_FILTER_THRESH       (153227) /* ((153,227 um/s^2)/count) * 1 */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_BMA250
#define YAS_ACC_DEFAULT_FILTER_THRESH       (76612)  /* ((38,306 um/s^2)/count) * 2  */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_KXSD9
#define YAS_ACC_DEFAULT_FILTER_THRESH       (239460) /* ((11,973 um/s^2)/count) * 20 */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_KXTE9
#define YAS_ACC_DEFAULT_FILTER_THRESH       (612909) /* ((612,909 um/s^2)/count) * 1 */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_KXTF9
#define YAS_ACC_DEFAULT_FILTER_THRESH       (19152)  /* ((9,576 um/s^2)/count) * 2   */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_KXUD9
#define YAS_ACC_DEFAULT_FILTER_THRESH       (215514) /* ((11.973 um/s^2)/count * 18  */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_LIS331DL
#define YAS_ACC_DEFAULT_FILTER_THRESH       (176518) /* ((176.518 um/s^2)/count * 1  */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_LIS331DLH
#define YAS_ACC_DEFAULT_FILTER_THRESH       (95760)  /* ((9.576 um/s^2)/count * 10   */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_LIS331DLM
#define YAS_ACC_DEFAULT_FILTER_THRESH       (306454) /* ((153,227 um/s^2)/count * 2  */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_LIS3DH
#define YAS_ACC_DEFAULT_FILTER_THRESH       (76608)  /* ((9,576 um/s^2)/count * 8    */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_MMA8452Q
#define YAS_ACC_DEFAULT_FILTER_THRESH       (19152)  /* ((9.576 um/s^2)/count * 2    */
#elif YAS_ACC_DRIVER == YAS_ACC_DRIVER_MMA8453Q
#define YAS_ACC_DEFAULT_FILTER_THRESH       (38306)  /* ((38,306 um/s^2)/count * 1   */
#else
#error "unknown accelerometer"
#endif

/*----------------------------------------------------------------------------*/
/*                    Geomagnetic Calibration Configuration                   */
/*----------------------------------------------------------------------------*/

#define YAS_DEFAULT_MAGCALIB_THRESHOLD      (1)
/* Default distortion values are needed to be changed depends on device. */
/* Distortion values should not be set here, please set distortion values on the init.rc file.*/
#define YAS_DEFAULT_MAGCALIB_DISTORTION	    (15)
#define YAS_DEFAULT_MAGCALIB_SHAPE          (0)
#define YAS_MAGCALIB_SHAPE_NUM              (2)
#undef YAS_MAG_MANUAL_OFFSET

/*----------------------------------------------------------------------------*/
/*                          Gyroscope  Configuration                          */
/*----------------------------------------------------------------------------*/

#if YAS_GYRO_DRIVER == YAS_GYRO_DRIVER_ITG3200
#elif YAS_GYRO_DRIVER == YAS_GYRO_DRIVER_L3G4200D
#define YAS_GYRO_FIFO_MAX                   (32)
#define YAS_GYRO_FIFO_WTM                   (24) /* 1-32 */
#else
#endif

/*----------------------------------------------------------------------------*/
/*                      Geomagnetic Filter Configuration                      */
/*----------------------------------------------------------------------------*/

#define YAS_MAG_MAX_FILTER_LEN              (30)
#define YAS_MAG_DEFAULT_FILTER_NOISE_X      (144) /* sd: 1200 nT */
#define YAS_MAG_DEFAULT_FILTER_NOISE_Y      (144) /* sd: 1200 nT */
#define YAS_MAG_DEFAULT_FILTER_NOISE_Z      (144) /* sd: 1200 nT */
#define YAS_MAG_DEFAULT_FILTER_LEN          (20)

#if !defined(CONFIG_MACH_AEGIS) && !defined(CONFIG_MACH_VIPER) \
	&& !defined(CONFIG_TIKAL_USCC)
#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS529
#define YAS_MAG_DEFAULT_FILTER_THRESH       (300)
#elif YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS530
#define YAS_MAG_DEFAULT_FILTER_THRESH       (100)
#else
#error "unknown magnetometer"
#endif
#endif
#endif
