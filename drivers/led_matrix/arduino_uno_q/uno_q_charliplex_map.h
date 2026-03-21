 /*   
 *   Copyright 2026 Shankar Sridhar
 *   SPDX-License-Identifier: Apache-2.0
 * 
 * Arduino UNO Q LED Matrix - Charlieplex Pin Mapping
 * 
 * Hardware: 8 rows × 13 columns = 104 LEDs
 * Control: 11 GPIO pins (PF0-PF10) via Charlieplexing
 * 
 * This mapping was discovered through systematic hardware testing
 * on actual Arduino UNO Q hardware - February 2026
 * 
 * LED Numbering: 1-104 (displayed on hardware)
 * Array Index: 0-103 (in code)
 * Grid: Row × 13 + Col = LED Index
 */

#ifndef CHARLIPLEX_MAP_H
#define CHARLIPLEX_MAP_H

#include <stdint.h>

/* Pin pair for Charlieplexing */
typedef struct {
    uint8_t anode_pin;    /* Pin to drive HIGH */
    uint8_t cathode_pin;  /* Pin to drive LOW */
} led_pin_pair_t;

/*
 * LED Charlieplex Mapping Table
 * 
 * Index: LED position (0-103)
 * Value: {anode_pin, cathode_pin}
 * 
 */
static const led_pin_pair_t charliplex_map[104] = {
    /* Row 0 (LED 1-13, Index 0-12) */
    {0, 1},   /* LED 1   - Row 0, Col 0  */
    {1, 0},   /* LED 2   - Row 0, Col 1  */
    {0, 2},   /* LED 3   - Row 0, Col 2  */
    {2, 0},   /* LED 4   - Row 0, Col 3  */
    {1, 2},   /* LED 5   - Row 0, Col 4  */
    {2, 1},   /* LED 6   - Row 0, Col 5  */
    {0, 3},   /* LED 7   - Row 0, Col 6  */
    {3, 0},   /* LED 8   - Row 0, Col 7  */
    {1, 3},   /* LED 9   - Row 0, Col 8  */
    {3, 1},   /* LED 10  - Row 0, Col 9  */
    {2, 3},   /* LED 11  - Row 0, Col 10 */
    {3, 2},   /* LED 12  - Row 0, Col 11 */
    {0, 4},   /* LED 13  - Row 0, Col 12 */
    
    /* Row 1 (LED 14-26, Index 13-25) */
    {4, 0},   /* LED 14  - Row 1, Col 0  */
    {1, 4},   /* LED 15  - Row 1, Col 1  */
    {4, 1},   /* LED 16  - Row 1, Col 2  */
    {2, 4},   /* LED 17  - Row 1, Col 3  */
    {4, 2},   /* LED 18  - Row 1, Col 4  */
    {3, 4},   /* LED 19  - Row 1, Col 5  */
    {4, 3},   /* LED 20  - Row 1, Col 6  */
    {0, 5},   /* LED 21  - Row 1, Col 7  */
    {5, 0},   /* LED 22  - Row 1, Col 8  */
    {1, 5},   /* LED 23  - Row 1, Col 9  */
    {5, 1},   /* LED 24  - Row 1, Col 10 */
    {2, 5},   /* LED 25  - Row 1, Col 11 */
    {5, 2},   /* LED 26  - Row 1, Col 12 */
    
    /* Row 2 (LED 27-39, Index 26-38) */
    {3, 5},   /* LED 27  - Row 2, Col 0  */
    {5, 3},   /* LED 28  - Row 2, Col 1  */
    {4, 5},   /* LED 29  - Row 2, Col 2  */
    {5, 4},   /* LED 30  - Row 2, Col 3  */
    {0, 6},   /* LED 31  - Row 2, Col 4  */
    {6, 0},   /* LED 32  - Row 2, Col 5  */
    {1, 6},   /* LED 33  - Row 2, Col 6  */
    {6, 1},   /* LED 34  - Row 2, Col 7  */
    {2, 6},   /* LED 35  - Row 2, Col 8  */
    {6, 2},   /* LED 36  - Row 2, Col 9  */
    {3, 6},   /* LED 37  - Row 2, Col 10 */
    {6, 3},   /* LED 38  - Row 2, Col 11 */
    {4, 6},   /* LED 39  - Row 2, Col 12 */
    
    /* Row 3 (LED 40-52, Index 39-51) */
    {6, 4},   /* LED 40  - Row 3, Col 0  */
    {5, 6},   /* LED 41  - Row 3, Col 1  */
    {6, 5},   /* LED 42  - Row 3, Col 2  */
    {0, 7},   /* LED 43  - Row 3, Col 3  */
    {7, 0},   /* LED 44  - Row 3, Col 4  */
    {1, 7},   /* LED 45  - Row 3, Col 5  */
    {7, 1},   /* LED 46  - Row 3, Col 6  */
    {2, 7},   /* LED 47  - Row 3, Col 7  */
    {7, 2},   /* LED 48  - Row 3, Col 8  */
    {3, 7},   /* LED 49  - Row 3, Col 9  */
    {7, 3},   /* LED 50  - Row 3, Col 10 */
    {4, 7},   /* LED 51  - Row 3, Col 11 */
    {7, 4},   /* LED 52  - Row 3, Col 12 */
    
    /* Row 4 (LED 53-65, Index 52-64) */
    {5, 7},   /* LED 53  - Row 4, Col 0  */
    {7, 5},   /* LED 54  - Row 4, Col 1  */
    {6, 7},   /* LED 55  - Row 4, Col 2  */
    {7, 6},   /* LED 56  - Row 4, Col 3  */
    {0, 8},   /* LED 57  - Row 4, Col 4  */
    {8, 0},   /* LED 58  - Row 4, Col 5  */
    {1, 8},   /* LED 59  - Row 4, Col 6  */
    {8, 1},   /* LED 60  - Row 4, Col 7  */
    {2, 8},   /* LED 61  - Row 4, Col 8  */
    {8, 2},   /* LED 62  - Row 4, Col 9  */
    {3, 8},   /* LED 63  - Row 4, Col 10 */
    {8, 3},   /* LED 64  - Row 4, Col 11 */
    {4, 8},   /* LED 65  - Row 4, Col 12 */
    
    /* Row 5 (LED 66-78, Index 65-77) */
    {8, 4},   /* LED 66  - Row 5, Col 0  */
    {5, 8},   /* LED 67  - Row 5, Col 1  */
    {8, 5},   /* LED 68  - Row 5, Col 2  */
    {6, 8},   /* LED 69  - Row 5, Col 3  */
    {8, 6},   /* LED 70  - Row 5, Col 4  */
    {7, 8},   /* LED 71  - Row 5, Col 5  */
    {8, 7},   /* LED 72  - Row 5, Col 6  */
    {0, 9},   /* LED 73  - Row 5, Col 7  */
    {9, 0},   /* LED 74  - Row 5, Col 8  */
    {1, 9},   /* LED 75  - Row 5, Col 9  */
    {9, 1},   /* LED 76  - Row 5, Col 10 */
    {2, 9},   /* LED 77  - Row 5, Col 11 */
    {9, 2},   /* LED 78  - Row 5, Col 12 */
    
    /* Row 6 (LED 79-91, Index 78-90) */
    {3, 9},   /* LED 79  - Row 6, Col 0  */
    {9, 3},   /* LED 80  - Row 6, Col 1  */
    {4, 9},   /* LED 81  - Row 6, Col 2  */
    {9, 4},   /* LED 82  - Row 6, Col 3  */
    {5, 9},   /* LED 83  - Row 6, Col 4  */
    {9, 5},   /* LED 84  - Row 6, Col 5  */
    {6, 9},   /* LED 85  - Row 6, Col 6  */
    {9, 6},   /* LED 86  - Row 6, Col 7  */
    {7, 9},   /* LED 87  - Row 6, Col 8  */
    {9, 7},   /* LED 88  - Row 6, Col 9  */
    {8, 9},   /* LED 89  - Row 6, Col 10 */
    {9, 8},   /* LED 90  - Row 6, Col 11 */
    {0, 10},  /* LED 91  - Row 6, Col 12 */
    
    /* Row 7 (LED 92-104, Index 91-103) */
    {10, 0},  /* LED 92  - Row 7, Col 0  */
    {1, 10},  /* LED 93  - Row 7, Col 1  */
    {10, 1},  /* LED 94  - Row 7, Col 2  */
    {2, 10},  /* LED 95  - Row 7, Col 3  */
    {10, 2},  /* LED 96  - Row 7, Col 4  */
    {3, 10},  /* LED 97  - Row 7, Col 5  */
    {10, 3},  /* LED 98  - Row 7, Col 6  */
    {4, 10},  /* LED 99  - Row 7, Col 7  */
    {10, 4},  /* LED 100 - Row 7, Col 8  */
    {5, 10},  /* LED 101 - Row 7, Col 9  */
    {10, 5},  /* LED 102 - Row 7, Col 10 */
    {6, 10},  /* LED 103 - Row 7, Col 11 */
    {10, 6},  /* LED 104 - Row 7, Col 12 */
};

#endif /* CHARLIPLEX_MAP_H */
