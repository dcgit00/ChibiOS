/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    STM32G4xx/hal_lld.c
 * @brief   STM32G4xx HAL subsystem low level driver source.
 *
 * @addtogroup HAL
 * @{
 */

#include "hal.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Number of thresholds in the wait states array.
 */
#define STM32_WS_THRESHOLDS             5

/**
 * @name    Registers reset values
 * @{
 */
#define STM32_PWR_VOSCR_RESET           0U
#define STM32_PWR_VMCR_RESET            0U
#define STM32_FLASH_ACR_RESET           (FLASH_ACR_WRHIGHFREQ_0     |       \
                                         FLASH_ACR_LATENCY_3WS)
#define STM32_RCC_CR_RESET              0x0000002BU
#define STM32_RCC_CFGR1_RESET           0U
#define STM32_RCC_CFGR2_RESET           0U
#define STM32_RCC_PLL1CFGR_RESET        0U
#define STM32_RCC_PLL2CFGR_RESET        0U
#define STM32_RCC_PLL3CFGR_RESET        0U
#define STM32_RCC_PLL1DIVR_RESET        0x01010280U
#define STM32_RCC_PLL1FRACR_RESET       0U
#define STM32_RCC_PLL2DIVR_RESET        0x01010280U
#define STM32_RCC_PLL2FRACR_RESET       0U
#define STM32_RCC_PLL3DIVR_RESET        0x01010280U
#define STM32_RCC_PLL3FRACR_RESET       0U
/** @} */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   CMSIS system core clock variable.
 * @note    It is declared in system_stm32g4xx.h.
 */
uint32_t SystemCoreClock = STM32_HCLK;

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   Post-reset clock configuration.
 */
const halclkcfg_t hal_clkcfg_reset = {
  .pwr_voscr            = STM32_PWR_VOSCR_RESET,
  .pwr_vmcr             = STM32_PWR_VMCR_RESET,
  .rcc_cr               = STM32_RCC_CR_RESET,
  .rcc_cfgr1            = STM32_RCC_CFGR1_RESET,
  .rcc_cfgr2            = STM32_RCC_CFGR2_RESET,
  .flash_acr            = STM32_FLASH_ACR_RESET,
  .plls = {
    [0] = {
      .cfgr             = STM32_RCC_PLL1CFGR_RESET,
      .divr             = STM32_RCC_PLL1DIVR_RESET,
      .frac             = STM32_RCC_PLL1FRACR_RESET
    },
    [1] = {
      .cfgr             = STM32_RCC_PLL2CFGR_RESET,
      .divr             = STM32_RCC_PLL2DIVR_RESET,
      .frac             = STM32_RCC_PLL2FRACR_RESET
    },
    [2] = {
      .cfgr             = STM32_RCC_PLL3CFGR_RESET,
      .divr             = STM32_RCC_PLL3DIVR_RESET,
      .frac             = STM32_RCC_PLL3FRACR_RESET
    }
  }
};

/**
 * @brief   Default clock configuration.
 */
const halclkcfg_t hal_clkcfg_default = {
  .pwr_voscr            = STM32_PWR_VOSCR,
  .pwr_vmcr             = STM32_PWR_VMCR,
  .rcc_cr               = 0U
#if STM32_ACTIVATE_PLL3
                        | RCC_CR_PLL3ON
#endif
#if STM32_ACTIVATE_PLL2
                        | RCC_CR_PLL2ON
#endif
#if STM32_ACTIVATE_PLL1
                        | RCC_CR_PLL1ON
#endif
#if STM32_HSE_ENABLED
                        | RCC_CR_HSEON
#endif
#if STM32_HSI48_ENABLED
                        | RCC_CR_HSI48ON
#endif
#if STM32_CSI_ENABLED
                        | RCC_CR_CSION
#endif
#if STM32_HSI_ENABLED
                        | STM32_HSIDIV | RCC_CR_HSION
#endif
                          ,
  .flash_acr            = FLASH_ACR_PRFTEN  | STM32_FLASHBITS,
  .plls = {
    [0] = {
      .cfgr             = STM32_PLL1REN     | STM32_PLL1QEN     |
                          STM32_PLL1PEN     | STM32_PLL1M       |
                          STM32_PLL1RGE     | STM32_PLL1VCOSEL  |
                          STM32_PLL1SRC,
      .divr             = STM32_PLL1R       | STM32_PLL1Q       |
                          STM32_PLL1P       | STM32_PLL1N,
      .frac             = STM32_RCC_PLL1FRACR_RESET
    },
    [1] = {
      .cfgr             = STM32_PLL2REN     | STM32_PLL2QEN     |
                          STM32_PLL2PEN     | STM32_PLL2M       |
                          STM32_PLL2RGE     | STM32_PLL2VCOSEL  |
                          STM32_PLL2SRC,
      .divr             = STM32_PLL2R       | STM32_PLL2Q       |
                          STM32_PLL2P       | STM32_PLL2N,
      .frac             = STM32_RCC_PLL2FRACR_RESET
    },
    [2] = {
      .cfgr             = STM32_PLL3REN     | STM32_PLL3QEN     |
                          STM32_PLL3PEN     | STM32_PLL3M       |
                          STM32_PLL3RGE     | STM32_PLL3VCOSEL  |
                          STM32_PLL3SRC,
      .divr             = STM32_PLL3R       | STM32_PLL3Q       |
                          STM32_PLL3P       | STM32_PLL3N,
      .frac             = STM32_RCC_PLL3FRACR_RESET
    }
  }
};
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   Dynamic clock points for this device.
 */
static halfreq_t clock_points[CLK_ARRAY_SIZE] = {
  [CLK_SYSCLK]          = STM32_SYSCLK,
  [CLK_PLL1PCLK]        = STM32_PLL1_P_CLKOUT,
  [CLK_PLL1QCLK]        = STM32_PLL1_Q_CLKOUT,
  [CLK_PLL1RCLK]        = STM32_PLL1_R_CLKOUT,
  [CLK_PLL2PCLK]        = STM32_PLL2_P_CLKOUT,
  [CLK_PLL2QCLK]        = STM32_PLL2_Q_CLKOUT,
  [CLK_PLL2RCLK]        = STM32_PLL2_R_CLKOUT,
  [CLK_PLL3PCLK]        = STM32_PLL3_P_CLKOUT,
  [CLK_PLL3QCLK]        = STM32_PLL3_Q_CLKOUT,
  [CLK_PLL3RCLK]        = STM32_PLL3_R_CLKOUT,
  [CLK_HCLK]            = STM32_HCLK,
  [CLK_PCLK1]           = STM32_PCLK1,
  [CLK_PCLK1TIM]        = STM32_TIMP1CLK,
  [CLK_PCLK2]           = STM32_PCLK2,
  [CLK_PCLK2TIM]        = STM32_TIMP2CLK,
  [CLK_PCLK3]           = STM32_PCLK3,
  [CLK_MCO1]            = STM32_MCO2CLK,
  [CLK_MCO2]            = STM32_MCO1CLK,
  [CLK_HSI48]           = STM32_HSI48CLK
};

/**
 * @brief   Type of a structure representing system limits.
 */
typedef struct {
  halfreq_t             sysclk_max;
  halfreq_t             flash_thresholds[STM32_WS_THRESHOLDS];
} system_limits_t;

/**
 * @brief   System limits for VOS range 0.
 */
static const system_limits_t vos_range0 = {
  .sysclk_max           = STM32_VOS0_SYSCLK_MAX,
  .flash_thresholds     = {STM32_VOS0_0WS_THRESHOLD, STM32_VOS0_1WS_THRESHOLD,
                           STM32_VOS0_2WS_THRESHOLD, STM32_VOS0_3WS_THRESHOLD,
                           STM32_VOS0_4WS_THRESHOLD}
};

/**
 * @brief   System limits for VOS range 1.
 */
static const system_limits_t vos_range1 = {
  .sysclk_max           = STM32_VOS1_SYSCLK_MAX,
  .flash_thresholds     = {STM32_VOS1_0WS_THRESHOLD, STM32_VOS1_1WS_THRESHOLD,
                           STM32_VOS1_2WS_THRESHOLD, STM32_VOS1_3WS_THRESHOLD,
                           STM32_VOS1_4WS_THRESHOLD}
};

/**
 * @brief   System limits for VOS range 2.
 */
static const system_limits_t vos_range2 = {
  .sysclk_max           = STM32_VOS2_SYSCLK_MAX,
  .flash_thresholds     = {STM32_VOS2_0WS_THRESHOLD, STM32_VOS2_1WS_THRESHOLD,
                           STM32_VOS2_2WS_THRESHOLD, STM32_VOS2_3WS_THRESHOLD,
                           STM32_VOS2_4WS_THRESHOLD}
};

/**
 * @brief   System limits for VOS range 3.
 */
static const system_limits_t vos_range3 = {
  .sysclk_max           = STM32_VOS3_SYSCLK_MAX,
  .flash_thresholds     = {STM32_VOS3_0WS_THRESHOLD, STM32_VOS3_1WS_THRESHOLD,
                           STM32_VOS3_2WS_THRESHOLD, STM32_VOS3_3WS_THRESHOLD,
                           STM32_VOS3_4WS_THRESHOLD}
};
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#include "stm32_bd.inc"

/**
 * @brief   Safe setting of flash ACR register.
 *
 * @param[in] acr       value for the ACR register
 */
__STATIC_INLINE void flash_set_acr(uint32_t acr) {

  FLASH->ACR = acr;
  while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != (acr & FLASH_ACR_LATENCY_Msk)) {
    /* Waiting for flash wait states setup.*/
  }
}

/**
 * @brief   Configures the PWR unit.
 * @note    CR1, CR2 and CR5 are not initialized inside this function.
 */
__STATIC_INLINE void hal_lld_set_static_pwr(void) {

  /* PWR clock enabled.*/
//  rccEnablePWRInterface(false);

  /* Static PWR configurations.*/
//  PWR->VOSCR    = STM32_PWR_VOSCR;
  PWR->BDCR     = STM32_PWR_BDCR;
  PWR->UCPDR    = STM32_PWR_UCPDR;
  PWR->SCCR     = STM32_PWR_SCCR;
//  PWR->VMCR     = STM32_PWR_VMCR;
  PWR->USBSCR   = STM32_PWR_USBSCR;
  PWR->WUCR     = STM32_PWR_WUCR;
  PWR->IORETR   = STM32_PWR_IORETR;
  PWR->SECCFGR  = STM32_PWR_SECCFGR;
  PWR->PRIVCFGR = STM32_PWR_PRIVCFGR;
}

/**
 * @brief   Initializes static muxes and dividers.
 */
__STATIC_INLINE void hal_lld_set_static_clocks(void) {

  /* Clock-related settings (dividers, MCO etc).*/
  RCC->CFGR1  = STM32_MCO2SEL     | STM32_MCO2PRE     |
                STM32_MCO1SEL     | STM32_MCO1PRE     |
                STM32_TIMPRE      | STM32_RTCPRE      |
                STM32_STOPWUCK    | STM32_STOPKERWUCK;
  RCC->CFGR2  = STM32_PPRE3       | STM32_PPRE2       |
                STM32_PPRE1       | STM32_HPRE;

  /* CCIPR registers initialization, note.*/
  RCC->CCIPR1 = STM32_TIMICSEL    | STM32_USART10SEL  |
                STM32_UART9SEL    | STM32_UART9SEL    |
                STM32_UART8SEL    | STM32_UART7SEL    |
                STM32_USART6SEL   | STM32_UART5SEL    |
                STM32_UART4SEL    | STM32_USART3SEL   |
                STM32_USART2SEL   | STM32_USART1SEL;
  RCC->CCIPR2 = STM32_LPTIM6SEL   | STM32_LPTIM5SEL   |
                STM32_LPTIM4SEL   | STM32_LPTIM3SEL   |
                STM32_LPTIM2SEL   | STM32_LPTIM1SEL   |
                STM32_UART12SEL   | STM32_USART11SEL;
  RCC->CCIPR3 = STM32_LPUART1SEL  | STM32_SPI6SEL     |
                STM32_SPI5SEL     | STM32_SPI4SEL     |
                STM32_SPI3SEL     | STM32_SPI2SEL     |
                STM32_SPI1SEL;
  RCC->CCIPR4 = STM32_I3C1SEL     | STM32_I2C4SEL     |
                STM32_I2C3SEL     | STM32_I2C2SEL     |
                STM32_I2C1SEL     | STM32_SDMMC2SEL   |
                STM32_SDMMC1SEL   | STM32_USBSEL      |
                STM32_SYSTICKSEL  | STM32_OSPISEL;
  RCC->CCIPR5 = STM32_CKPERSEL    | STM32_SAI2SEL     |
                STM32_SAI1SEL     |
#if STM32_FDCANSEL != STM32_FDCANSEL_IGNORE
                STM32_FDCANSEL    |
#endif
#if STM32_CECSEL != STM32_CECSEL_IGNORE
                STM32_CECSEL      |
#endif
#if STM32_RNGSEL != STM32_RNGSEL_IGNORE
                STM32_RNGSEL      |
#endif
#if STM32_DACSEL != STM32_DACSEL_IGNORE
                STM32_DACSEL      |
#endif
                STM32_ADCDACSEL;
}

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   Recalculates the clock tree frequencies.
 *
 * @param[in] ccp       pointer to clock a @p halclkcfg_t structure
 * @return              The frequency calculation result.
 * @retval false        if the clock settings look valid
 * @retval true         if the clock settings look invalid
 *
 * @notapi
 */
static bool hal_lld_clock_check_tree(const halclkcfg_t *ccp) {
  static const uint32_t hprediv[16] = {1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U,
                                       2U, 4U, 8U, 16U, 64U, 128U, 256U, 512U};
  static const uint32_t pprediv[16] = {1U, 1U, 1U, 1U, 2U, 4U, 8U, 16U};
  const system_limits_t *slp;
  uint32_t n, flashws;
  halfreq_t hsiclk = 0U, hsi48clk = 0U, csiclk = 0U, hseclk = 0U;
  halfreq_t pll1selclk, pll2selclk, pll3selclk;
  halfreq_t pll1pclk = 0U, pll1qclk = 0U, pll1rclk = 0U;
  halfreq_t pll2pclk = 0U, pll2qclk = 0U, pll2rclk = 0U;
  halfreq_t pll3pclk = 0U, pll3qclk = 0U, pll3rclk = 0U;
  halfreq_t sysclk, hclk, pclk1, pclk2, pclk3, pclk1tim, pclk2tim, mco1clk, mco2clk;

  /* System limits based on desired VOS settings.*/
  switch (ccp->pwr_voscr) {
  case STM32_VOS_RANGE3:
    slp = &vos_range3;
    break;
  case STM32_VOS_RANGE2:
    slp = &vos_range2;
    break;
  case STM32_VOS_RANGE1:
    slp = &vos_range1;
    break;
  case STM32_VOS_RANGE0:
    slp = &vos_range0;
    break;
  default:
    return true;
  }

  /* HSE clock.*/
  if ((ccp->rcc_cr & RCC_CR_HSEON) != 0U) {
    hseclk = STM32_HSECLK;
  }

  /* HSI48 clock after divider.*/
  if ((ccp->rcc_cr & RCC_CR_HSI48ON) != 0U) {
    hsi48clk = STM32_HSI48CLK;
  }

  /* HSI clock after divider.*/
  if ((ccp->rcc_cr & RCC_CR_HSION) != 0U) {
    hsiclk = STM32_HSI64CLK / (1U << ((ccp->rcc_cr & STM32_HSIDIV_MASK) >> STM32_HSIDIV_POS));
  }

  /* CSI clock.*/
  if ((ccp->rcc_cr & RCC_CR_CSION) != 0U) {
    csiclk = STM32_CSICLK;
  }

  /* PLL1 MUX clock.*/
  switch (ccp->plls[0].cfgr & STM32_PLLSRC_MASK) {
  case STM32_PLL1SRC_HSI:
    pll1selclk = hsiclk;
    break;
  case STM32_PLL1SRC_CSI:
    pll1selclk = csiclk;
    break;
  case STM32_PLL1SRC_HSE:
    pll1selclk = hseclk;
    break;
  default:
    pll1selclk = 0U;
  }

  /* PLL2 MUX clock.*/
  switch (ccp->plls[1].cfgr & STM32_PLLSRC_MASK) {
  case STM32_PLL2SRC_HSI:
    pll2selclk = hsiclk;
    break;
  case STM32_PLL2SRC_CSI:
    pll2selclk = csiclk;
    break;
  case STM32_PLL2SRC_HSE:
    pll2selclk = hseclk;
    break;
  default:
    pll2selclk = 0U;
  }

  /* PLL3 MUX clock.*/
  switch (ccp->plls[2].cfgr & STM32_PLLSRC_MASK) {
  case STM32_PLL3SRC_HSI:
    pll3selclk = hsiclk;
    break;
  case STM32_PLL3SRC_CSI:
    pll3selclk = csiclk;
    break;
  case STM32_PLL3SRC_HSE:
    pll3selclk = hseclk;
    break;
  default:
    pll3selclk = 0U;
  }

  /* PLL1 outputs.*/
  if ((ccp->rcc_cr & RCC_CR_PLL1ON) != 0U) {
    uint32_t mdiv, ndiv;
    halfreq_t vcoclk;

    /* PLL1 M divider.*/
    mdiv = (ccp->plls[0].cfgr & STM32_PLLM_MASK) >> STM32_PLLM_POS;

    /* PLL1 N divider.*/
    ndiv = ((ccp->plls[0].divr & STM32_PLLN_MASK) >> STM32_PLLN_POS) + 1U;
    if (ndiv < STM32_PLL1N_VALUE_MIN) {
      return true;
    }

    /* PLL1 VCO frequency.*/
    vcoclk = (pll1selclk / (halfreq_t)mdiv) * (halfreq_t)ndiv;
    if ((vcoclk < STM32_PLLVCO_MIN) || (vcoclk > STM32_PLLVCO_MAX)) {
      return true;
    }

    /* PLL1 P output frequency.*/
    if ((ccp->plls[0].cfgr & STM32_PLLPEN) != 0U) {
      n = ((ccp->plls[0].divr & STM32_PLLP_MASK) >> STM32_PLLP_POS) + 1U;
      if ((n & 1U) != 0U) {  /* Cannot be odd, PLL1P-only.*/
        return true;
      }
      pll1pclk = vcoclk / n;
      if ((pll1pclk < STM32_PLLP_MIN) || (pll1pclk > STM32_PLLP_MAX)) {
        return true;
      }
    }

    /* PLL1 Q output frequency.*/
    if ((ccp->plls[0].cfgr & STM32_PLLQEN) != 0U) {
      n = ((ccp->plls[0].divr & STM32_PLLQ_MASK) >> STM32_PLLQ_POS) + 1U;
      pll1qclk = vcoclk / n;
      if ((pll1qclk < STM32_PLLQ_MIN) || (pll1qclk > STM32_PLLQ_MAX)) {
        return true;
      }
    }

    /* PLL1 R output frequency.*/
    if ((ccp->plls[0].cfgr & STM32_PLLREN) != 0U) {
      n = ((ccp->plls[0].divr & STM32_PLLR_MASK) >> STM32_PLLR_POS) + 1U;
      pll1rclk = vcoclk / n;
      if ((pll1rclk < STM32_PLLR_MIN) || (pll1qclk > STM32_PLLR_MAX)) {
        return true;
      }
    }
  }

  /* PLL2 outputs.*/
  if ((ccp->rcc_cr & RCC_CR_PLL2ON) != 0U) {
    uint32_t mdiv, ndiv;
    halfreq_t vcoclk;

    /* PLL2 M divider.*/
    mdiv = (ccp->plls[1].cfgr & STM32_PLLM_MASK) >> STM32_PLLM_POS;

    /* PLL2 N divider.*/
    ndiv = ((ccp->plls[1].divr & STM32_PLLN_MASK) >> STM32_PLLN_POS) + 1U;
    if (ndiv < STM32_PLL2N_VALUE_MIN) {
      return true;
    }

    /* PLL2 VCO frequency.*/
    vcoclk = (pll2selclk / (halfreq_t)mdiv) * (halfreq_t)ndiv;
    if ((vcoclk < STM32_PLLVCO_MIN) || (vcoclk > STM32_PLLVCO_MAX)) {
      return true;
    }

    /* PLL2 P output frequency.*/
    if ((ccp->plls[1].cfgr & STM32_PLLPEN) != 0U) {
      n = ((ccp->plls[1].divr & STM32_PLLP_MASK) >> STM32_PLLP_POS) + 1U;
      pll2pclk = vcoclk / n;
      if ((pll2pclk < STM32_PLLP_MIN) || (pll2pclk > STM32_PLLP_MAX)) {
        return true;
      }
    }

    /* PLL2 Q output frequency.*/
    if ((ccp->plls[1].cfgr & STM32_PLLQEN) != 0U) {
      n = ((ccp->plls[1].divr & STM32_PLLQ_MASK) >> STM32_PLLQ_POS) + 1U;
      pll2qclk = vcoclk / n;
      if ((pll2qclk < STM32_PLLQ_MIN) || (pll2qclk > STM32_PLLQ_MAX)) {
        return true;
      }
    }

    /* PLL2 R output frequency.*/
    if ((ccp->plls[1].cfgr & STM32_PLLREN) != 0U) {
      n = ((ccp->plls[1].divr & STM32_PLLR_MASK) >> STM32_PLLR_POS) + 1U;
      pll2rclk = vcoclk / n;
      if ((pll2rclk < STM32_PLLR_MIN) || (pll2qclk > STM32_PLLR_MAX)) {
        return true;
      }
    }
  }

  /* PLL3 outputs.*/
  if ((ccp->rcc_cr & RCC_CR_PLL3ON) != 0U) {
    uint32_t mdiv, ndiv;
    halfreq_t vcoclk;

    /* PLL3 M divider.*/
    mdiv = (ccp->plls[2].cfgr & STM32_PLLM_MASK) >> STM32_PLLM_POS;

    /* PLL3 N divider.*/
    ndiv = ((ccp->plls[2].divr & STM32_PLLN_MASK) >> STM32_PLLN_POS) + 1U;
    if (ndiv < STM32_PLL3N_VALUE_MIN) {
      return true;
    }

    /* PLL3 VCO frequency.*/
    vcoclk = (pll3selclk / (halfreq_t)mdiv) * (halfreq_t)ndiv;
    if ((vcoclk < STM32_PLLVCO_MIN) || (vcoclk > STM32_PLLVCO_MAX)) {
      return true;
    }

    /* PLL3 P output frequency.*/
    if ((ccp->plls[2].cfgr & STM32_PLLPEN) != 0U) {
      n = ((ccp->plls[2].divr & STM32_PLLP_MASK) >> STM32_PLLP_POS) + 1U;
      pll3pclk = vcoclk / n;
      if ((pll3pclk < STM32_PLLP_MIN) || (pll3pclk > STM32_PLLP_MAX)) {
        return true;
      }
    }

    /* PLL3 Q output frequency.*/
    if ((ccp->plls[2].cfgr & STM32_PLLQEN) != 0U) {
      n = ((ccp->plls[2].divr & STM32_PLLQ_MASK) >> STM32_PLLQ_POS) + 1U;
      pll3qclk = vcoclk / n;
      if ((pll3qclk < STM32_PLLQ_MIN) || (pll3qclk > STM32_PLLQ_MAX)) {
        return true;
      }
    }

    /* PLL3 R output frequency.*/
    if ((ccp->plls[2].cfgr & STM32_PLLREN) != 0U) {
      n = ((ccp->plls[2].divr & STM32_PLLR_MASK) >> STM32_PLLR_POS) + 1U;
      pll3rclk = vcoclk / n;
      if ((pll3rclk < STM32_PLLR_MIN) || (pll3qclk > STM32_PLLR_MAX)) {
        return true;
      }
    }
  }

  /* SYSCLK frequency.*/
  switch (ccp->rcc_cfgr1 & STM32_SW_MASK) {
  case STM32_SW_HSI:
    sysclk = hsiclk;
    break;
  case STM32_SW_CSI:
    sysclk = csiclk;
    break;
  case STM32_SW_HSE:
    sysclk = hseclk;
    break;
  case STM32_SW_PLL1P:
    sysclk = pll1pclk;
    break;
  default:
    sysclk = 0U;
  }

  if (sysclk > slp->sysclk_max) {
    return true;
  }

  /* HCLK frequency.*/
  hclk = sysclk / hprediv[(ccp->rcc_cfgr2 & RCC_CFGR2_HPRE_Msk) >> RCC_CFGR2_HPRE_Pos];

  /* PPRE1 frequency.*/
  n = pprediv[(ccp->rcc_cfgr2 & RCC_CFGR2_PPRE1_Msk) >> RCC_CFGR2_PPRE1_Pos];
  pclk1 = hclk / n;
  if (n < 2) {
    pclk1tim = pclk1;
  }
  else {
    pclk1tim = pclk1 * 2U;
  }

  /* PPRE2 frequency.*/
  n = pprediv[(ccp->rcc_cfgr2 & RCC_CFGR2_PPRE2_Msk) >> RCC_CFGR2_PPRE2_Pos];
  pclk2 = hclk / n;
  if (n < 2) {
    pclk2tim = pclk2;
  }
  else {
    pclk2tim = pclk2 * 2U;
  }

  /* PPRE3 frequency.*/
  n = pprediv[(ccp->rcc_cfgr2 & RCC_CFGR2_PPRE3_Msk) >> RCC_CFGR2_PPRE3_Pos];
  pclk3 = hclk / n;

  /* MCO1 clock.*/
  switch (ccp->rcc_cfgr1 & STM32_MCO1SEL_MASK) {
  case STM32_MCO1SEL_HSI:
    mco1clk = hsiclk;
    break;
  case STM32_MCO1SEL_LSE:
    mco1clk = STM32_LSECLK;
    break;
  case STM32_MCO1SEL_HSE:
    mco1clk = hseclk;
    break;
  case STM32_MCO1SEL_PLL1P:
    mco1clk = pll1pclk;
    break;
  case STM32_MCO1SEL_HSI48:
    mco1clk = STM32_HSI48CLK;
    break;
  default:
    mco1clk = 0U;
  }
  n = (ccp->rcc_cfgr1 & STM32_MCO1PRE_MASK) >> STM32_MCO1PRE_POS;
  if (n == 0U) {
    return true;
  }
  mco1clk /= n;

  /* MCO2 clock.*/
  switch (ccp->rcc_cfgr1 & STM32_MCO2SEL_MASK) {
  case STM32_MCO2SEL_SYSCLK:
    mco2clk = sysclk;
    break;
  case STM32_MCO2SEL_PLL2P:
    mco2clk = pll2pclk;
    break;
  case STM32_MCO2SEL_HSE:
    mco2clk = hseclk;
    break;
  case STM32_MCO2SEL_PLL1P:
    mco2clk = pll1pclk;
    break;
  case STM32_MCO2SEL_CSI:
    mco2clk = csiclk;
    break;
  case STM32_MCO2SEL_LSI:
    mco2clk = STM32_LSICLK;
    break;
  default:
    mco2clk = 0U;
  }
  n = (ccp->rcc_cfgr1 & STM32_MCO2PRE_MASK) >> STM32_MCO2PRE_POS;
  if (n == 0U) {
    return true;
  }
  mco2clk /= n;

  /* Flash settings.*/
  flashws = ((ccp->flash_acr & FLASH_ACR_LATENCY_Msk) >> FLASH_ACR_LATENCY_Pos);
  if (flashws >= STM32_WS_THRESHOLDS) {
    return true;
  }
  if (hclk > slp->flash_thresholds[flashws]) {
    return true;
  }

  /* Writing out results.*/
  clock_points[CLK_SYSCLK]   = sysclk;
  clock_points[CLK_PLL1PCLK] = pll1pclk;
  clock_points[CLK_PLL1QCLK] = pll1qclk;
  clock_points[CLK_PLL1RCLK] = pll1rclk;
  clock_points[CLK_PLL2PCLK] = pll2pclk;
  clock_points[CLK_PLL2QCLK] = pll2qclk;
  clock_points[CLK_PLL2RCLK] = pll2rclk;
  clock_points[CLK_PLL3PCLK] = pll3pclk;
  clock_points[CLK_PLL3QCLK] = pll3qclk;
  clock_points[CLK_PLL3RCLK] = pll3rclk;
  clock_points[CLK_HCLK]     = hclk;
  clock_points[CLK_PCLK1]    = pclk1;
  clock_points[CLK_PCLK1TIM] = pclk1tim;
  clock_points[CLK_PCLK2]    = pclk2;
  clock_points[CLK_PCLK2TIM] = pclk2tim;
  clock_points[CLK_PCLK3]    = pclk3;
  clock_points[CLK_MCO1]     = mco1clk;
  clock_points[CLK_MCO2]     = mco2clk;
  clock_points[CLK_HSI48]    = hsi48clk;

  return false;
}

/**
 * @brief   Switches to a different clock configuration.
 *
 * @param[in] ccp       pointer to clock a @p halclkcfg_t structure
 * @return              The clock switch result.
 * @retval false        if the clock switch succeeded
 * @retval true         if the clock switch failed
 *
 * @notapi
 */
static bool hal_lld_clock_raw_switch(const halclkcfg_t *ccp) {

#if 0
  /* Restoring default PWR settings related clocks and sleep modes.*/
  PWR->CR1 = PWR_CR1_VOS_0;

  /* Waiting for all regulator status bits to be cleared, this means that
     power levels are stable.*/
  while ((PWR->SR2 & (PWR_SR2_VOSF | PWR_SR2_REGLPF)) != 0U) {
    /* Waiting for the regulator to be ready.*/
  }
#endif

  /* HSI could be not activated, activating it taking care to not disturb
     other clocks yet, not touching the divider.*/
  RCC->CR |= RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {
    /* Waiting for HSI activation.*/
  }

  /* Switching to HSI as clock source as in a post-reset situation.*/
  RCC->CFGR1 = STM32_RCC_CFGR1_RESET;
  RCC->CFGR2 = STM32_RCC_CFGR2_RESET;
  while ((RCC->CFGR1 & STM32_SWS_MASK) != STM32_SWS_HSI) {
    /* Wait until HSI is selected.*/
  }

  /* Resetting the whole RCC_CR register, shutting down everything but HSI.*/
  RCC->CR = STM32_RCC_CR_RESET;
  while ((RCC->CR & RCC_CR_HSIDIVF) == 0U) {
    /* Waiting for new HSIDIV setting to be propagated.*/
  }

  /* Resetting flash ACR settings to the default value.*/
  flash_set_acr(STM32_FLASH_ACR_RESET);

  /* HSE setup, if required, before starting the PLL.*/
  if ((ccp->rcc_cr & RCC_CR_HSEON) != 0U) {
    hse_enable();
  }

  /* HSI48 setup, if required, before starting the PLL.*/
  if ((ccp->rcc_cr & RCC_CR_HSI48ON) != 0U) {
    hsi48_enable();
  }

  /* CSI setup, if required, before starting the PLL.*/
  if ((ccp->rcc_cr & RCC_CR_CSION) != 0U) {
    csi_enable();
  }

  /* PLLs setup.*/
  if ((ccp->rcc_cr & RCC_CR_PLL1ON) != 0U) {
    pll1_activate(ccp->plls[0].cfgr, ccp->plls[0].divr, 0U);
  }
  if ((ccp->rcc_cr & RCC_CR_PLL2ON) != 0U) {
    pll2_activate(ccp->plls[1].cfgr, ccp->plls[1].divr, 0U);
  }
  if ((ccp->rcc_cr & RCC_CR_PLL3ON) != 0U) {
    pll3_activate(ccp->plls[2].cfgr, ccp->plls[2].divr, 0U);
  }

  /* PLL activation polling if required.*/
  while (true) {
    if (((ccp->rcc_cr & RCC_CR_PLLON) != 0U) && pll_not_locked()) {
      continue;
    }
    break;
  }

  /* MCO and bus dividers first.*/
  RCC->CFGR = (RCC->CFGR & RCC_CFGR_SW_Msk) | (ccp->rcc_cfgr & ~RCC_CFGR_SW_Msk);

  /* Final flash ACR settings.*/
  flash_set_acr(ccp->flash_acr);

  /* Final PWR modes.*/
  PWR->CR1 = ccp->pwr_cr1;
  PWR->CR2 = ccp->pwr_cr2;
  PWR->CR5 = ccp->pwr_cr5;

  /* Waiting for the correct regulator state.*/
  if ((ccp->pwr_cr1 & PWR_CR1_LPR) == 0U) {
    /* Main mode selected.*/

    while ((PWR->SR2 & PWR_SR2_REGLPF) != 0U) {
      /* Waiting for the regulator to be in main mode.*/
    }
  }
  else {
    /* Low power mode selected.*/

    while ((PWR->SR2 & PWR_SR2_REGLPF) == 0U) {
      /* Waiting for the regulator to be in low power mode.*/
    }
  }

  /* Switching to the final clock source.*/
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | (ccp->rcc_cfgr & RCC_CFGR_SW_Msk);
  while ((RCC->CFGR & RCC_CFGR_SWS) != ((ccp->rcc_cfgr & RCC_CFGR_SW_Msk) << RCC_CFGR_SWS_Pos)) {
    /* Waiting for clock switch.*/
  }

  /* If HSI16 is not in configuration then it is finally shut down.*/
  if ((ccp->rcc_cr & RCC_CR_HSION) == 0U) {
    hsi16_disable();
  }

  return false;
}
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void hal_lld_init(void) {

  /* DMA subsystems initialization.*/
#if defined(STM32_DMA_REQUIRED)
  dmaInit();
#endif

  /* NVIC initialization.*/
  nvicInit();

  /* IRQ subsystem initialization.*/
  irqInit();
}

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   STM32L4xx clocks and PLL initialization.
 * @note    All the involved constants come from the file @p board.h.
 * @note    This function should be invoked just after the system reset.
 *
 * @special
 */
void stm32_clock_init(void) {

#if !STM32_NO_INIT
  /* Reset of all peripherals.
     Note, GPIOs are not reset because initialized before this point in
     board files.*/
  rccResetAHB1(~0);
  rccResetAHB2(~STM32_GPIO_EN_MASK);
  rccResetAHB3(~0);
  rccResetAPB1R1(~0);
  rccResetAPB1R2(~0);
  rccResetAPB2(~0);

  /* SYSCFG clock enabled here because it is a multi-functional unit shared
     among multiple drivers.*/
  rccEnableAPB2(RCC_APB2ENR_SYSCFGEN, false);

  /* RTC APB clock enable.*/
#if (HAL_USE_RTC == TRUE) && defined(RCC_APB1ENR1_RTCAPBEN)
  rccEnableAPB1R1(RCC_APB1ENR1_RTCAPBEN, true)
#endif

  /* Static PWR configurations.*/
  hal_lld_set_static_pwr();

  /* Backup domain made accessible.*/
  PWR->CR1 |= PWR_CR1_DBP;

  /* Backup domain reset.*/
  bd_reset();

  /* Static clocks setup.*/
  lse_init();
  lsi_init();

  /* Static clocks setup.*/
  hal_lld_set_static_clocks();

  /* Selecting the default clock/power/flash configuration.*/
  if (hal_lld_clock_raw_switch(&hal_clkcfg_default)) {
    osalSysHalt("clkswc");
  }

  /* Backup domain initializations.*/
  bd_init();
#endif /* STM32_NO_INIT */
}

#else /* !defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */
void stm32_clock_init(void) {

#if !STM32_NO_INIT

  /* Reset of all peripherals.
     Note, GPIOs are not reset because initialized before this point in
     board files.*/
  rccResetAHB1(~0);
  rccResetAHB2(~STM32_GPIO_EN_MASK);
  rccResetAHB4(~0);
  rccResetAPB1L(~0);
  rccResetAPB1H(~0);
  rccResetAPB2(~0);
  rccResetAPB3(~0);

  /* RTC APB clock enable.*/
#if (HAL_USE_RTC == TRUE) && defined(RCC_APB1ENR1_RTCAPBEN)
  rccEnableAPB1R1(RCC_APB1ENR1_RTCAPBEN, true)
#endif

  /* Static PWR configurations.*/
  hal_lld_set_static_pwr();

  /* PWR core voltage and thresholds setup.*/
  PWR->VOSCR = STM32_PWR_VOSCR;
  while ((PWR->VOSSR & PWR_VOSSR_ACTVOSRDY) == 0U) {
    /* Wait until regulator is stable.*/
  }
  PWR->VMCR = STM32_PWR_VMCR;

  /* Backup domain reset.*/
  bd_reset();

  /* Clocks setup.*/
  lse_init();
  lsi_init();
  hsi_init();
  hsi48_init();
  hse_init();

  /* Backup domain initializations.*/
  bd_init();

  /* PLLs activation, if required.*/
  pll1_init();
  pll2_init();
  pll3_init();

  /* Static clocks setup.*/
  hal_lld_set_static_clocks();

  /* Set flash WS's for SYSCLK source.*/
  flash_set_acr(FLASH_ACR_PRFTEN | STM32_FLASHBITS);

  /* Switching to the configured SYSCLK source if it is different from HSI.*/
#if STM32_SW != STM32_SW_HSI
  RCC->CFGR1 |= STM32_SW;       /* Switches on the selected clock source.   */
//  while(1);
  while ((RCC->CFGR1 & STM32_SWS_MASK) != (STM32_SW << 3)) {
    /* Wait until SYSCLK is stable.*/
  }
#endif

  /* Cache enable.*/
  icache_init();

#endif /* STM32_NO_INIT */
}
#endif /* !defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   Switches to a different clock configuration
 *
 * @param[in] ccp       pointer to clock a @p halclkcfg_t structure
 * @return              The clock switch result.
 * @retval false        if the clock switch succeeded
 * @retval true         if the clock switch failed
 *
 * @notapi
 */
bool hal_lld_clock_switch_mode(const halclkcfg_t *ccp) {

  if (hal_lld_clock_check_tree(ccp)) {
    return true;
  }

  if (hal_lld_clock_raw_switch(ccp)) {
    return true;
  }

  /* Updating the CMSIS variable.*/
  SystemCoreClock = hal_lld_get_clock_point(CLK_HCLK);

  return false;
}

/**
 * @brief   Returns the frequency of a clock point in Hz.
 *
 * @param[in] clkpt     clock point to be returned
 * @return              The clock point frequency in Hz or zero if the
 *                      frequency is unknown.
 *
 * @notapi
 */
halfreq_t hal_lld_get_clock_point(halclkpt_t clkpt) {

  osalDbgAssert(clkpt < CLK_ARRAY_SIZE, "invalid clock point");

  return clock_points[clkpt];
}
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

/** @} */
