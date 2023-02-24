#include "bflb_port_boot2.h"
#include "bflb_flash.h"
#include "bl602_glb.h"
#include "bl602_ef_ctrl.h"
#include "bl602_hbn.h"
#include "bl602_sf_cfg_ext.h"
#include "bl602_glb.h"
#include "bl602_xip_sflash_ext.h"
#include "bl602_clock.h"

#include "bflb_gpio.h"
#include "bflb_common.h"
#include "bflb_efuse.h"
#include "tzc_sec_reg.h"

/****************************************************************************/ /**
 * @brief  clear mtimer
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
static void hal_boot2_mtimer_clear_time()
{
    *(volatile uint32_t *)(CLIC_CTRL_ADDR + CLIC_MTIME) = 0;
    *(volatile uint32_t *)(CLIC_CTRL_ADDR + CLIC_MTIME + 4) = 0;
}

/****************************************************************************/ /**
 * @brief  init clock
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_init_clock(void)
{
    hal_boot2_mtimer_clear_time();
    GLB_Set_MTimer_CLK(1, GLB_MTIMER_CLK_BCLK, Clock_System_Clock_Get(BL_SYSTEM_CLOCK_BCLK) / 1000 / 1000 - 1);
    GLB_Set_UART_CLK(ENABLE, HBN_UART_CLK_160M, 3);
}

/****************************************************************************/ /**
 * @brief  hal_boot2_custom
 *
 * @param  custom_param
 *
 * @return 0
 *
*******************************************************************************/
uint32_t hal_boot2_custom(void *custom_param)
{
    uint32_t sw_cfg, flash_pin_cfg;
    bflb_efuse_read_sw_usage(0, (uint32_t *)&sw_cfg);
    /* flash_pin_cfg
     * 0:internal flash with io switch,
     * 1:internal flash no io switch,
     * 2:GPIO 17-22
     * 3:GPIO 0-2 & 20-22
    */
    flash_pin_cfg = ((sw_cfg >> 16) & 0x3);
    if ((flash_pin_cfg == 0) || (flash_pin_cfg == 1)) {
        struct bflb_device_s *gpio = bflb_device_get_by_name("gpio");
        bflb_gpio_init(gpio, GPIO_PIN_20, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
        bflb_gpio_reset(gpio, GPIO_PIN_20);
    }
    return 0;
}

/****************************************************************************/ /**
 * @brief  hal_boot2_get_efuse_cfg
 *
 * @param  efuse_cfg
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_get_efuse_cfg(boot2_efuse_hw_config *efuse_cfg)
{
    uint32_t tmp;
    uint32_t rootClk;
    uint8_t hdiv = 0, bdiv = 0;

    /* save bclk fclk div and root clock sel */
    bdiv = GLB_Get_BCLK_Div();
    hdiv = GLB_Get_HCLK_Div();
    rootClk = BL_RD_REG(HBN_BASE, HBN_GLB);

    /* change root clock to rc32m for efuse operation */
    HBN_Set_ROOT_CLK_Sel(HBN_ROOT_CLK_RC32M);

    /* Get sign and aes type*/
    //EF_Ctrl_Read_Secure_Boot((EF_Ctrl_Sign_Type *)efuse_cfg->sign, (EF_Ctrl_SF_AES_Type *)efuse_cfg->encrypted);
    bflb_efuse_read_secure_boot((EF_Ctrl_Sign_Type *)efuse_cfg->sign, (EF_Ctrl_SF_AES_Type *)efuse_cfg->encrypted);
    /* Get hash:aes key slot 0 and slot1*/
    //EF_Ctrl_Read_AES_Key(0, (uint32_t *)efuse_cfg->pk_hash_cpu0, 8);
    bflb_efuse_read_aes_key(0, (uint8_t *)efuse_cfg->pk_hash_cpu0, 8);
    //EF_Ctrl_Read_Chip_ID(efuse_cfg->chip_id);
    bflb_efuse_get_chipid(efuse_cfg->chip_id);
    /* Get HBN check sign config */
    //EF_Ctrl_Read_Sw_Usage(0, &tmp);
    bflb_efuse_read_sw_usage(0, &tmp);
    efuse_cfg->hbn_check_sign = (tmp >> 22) & 0x01;

    /* restore bclk fclk div and root clock sel */
    GLB_Set_System_CLK_Div(hdiv, bdiv);
    BL_WR_REG(HBN_BASE, HBN_GLB, rootClk);
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

/****************************************************************************/ /**
 * @brief  reset sec eng clock
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_reset_sec_eng(void)
{
    GLB_AHB_Slave1_Reset(BL_AHB_SLAVE1_SEC);
    GLB_Set_PKA_CLK_Sel(GLB_PKA_CLK_PLL120M);
}

/****************************************************************************/ /**
 * @brief  system soft reset
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_sw_system_reset(void)
{
    GLB_SW_System_Reset();
}

/****************************************************************************/ /**
 * @brief  hal_boot2_set_psmode_status
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_set_psmode_status(uint32_t flag)
{
    HBN_Set_Status_Flag(flag);
}

/****************************************************************************/ /**
 * @brief  hal_boot2_get_psmode_status
 *
 * @param  None
 *
 * @return status flag
 *
*******************************************************************************/
uint32_t hal_boot2_get_psmode_status(void)
{
    return HBN_Get_Status_Flag();
}

/****************************************************************************/ /**
 * @brief  hal_boot2_get_user_fw
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
uint32_t hal_boot2_get_user_fw(void)
{
    return BL_RD_WORD(HBN_BASE + HBN_RSV0_OFFSET);
}

/****************************************************************************/ /**
 * @brief  hal_boot2_clr_user_fw
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_clr_user_fw(void)
{
    uint32_t *p = (uint32_t *)(HBN_BASE + HBN_RSV0_OFFSET);
    *p = 0;
}

/****************************************************************************/ /**
 * @brief  hal_boot2_sboot_finish
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void ATTR_TCM_SECTION hal_boot2_sboot_finish(void)
{
    uint32_t tmp_val;

    tmp_val = BL_RD_REG(TZC_SEC_BASE, TZC_SEC_TZC_ROM_CTRL);

    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, TZC_SEC_TZC_SBOOT_DONE, 0xf);

    BL_WR_REG(TZC_SEC_BASE, TZC_SEC_TZC_ROM_CTRL, tmp_val);
}

/****************************************************************************/ /**
 * @brief  uart gpio init
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_uart_gpio_init(void)
{
    GLB_GPIO_Type gpios[] = { GLB_GPIO_PIN_7, GLB_GPIO_PIN_16 };

    GLB_GPIO_Func_Init(GPIO_FUN_UART, gpios, 2);

    GLB_UART_Fun_Sel((GLB_GPIO_PIN_7 % 8), GLB_UART_SIG_FUN_UART0_RXD);
    GLB_UART_Fun_Sel((GLB_GPIO_PIN_16 % 8), GLB_UART_SIG_FUN_UART0_TXD);
}

/****************************************************************************/ /**
 * @brief  uart gpio init
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_debug_uart_gpio_init(void)
{
    GLB_GPIO_Type gpios[] = { GLB_GPIO_PIN_8 };

    GLB_GPIO_Func_Init(GPIO_FUN_UART, gpios, 1);

    GLB_UART_Sig_Swap_Set(UART_SIG_SWAP_GPIO8_GPIO15);
    GLB_UART_Fun_Sel(GLB_UART_SIG_4, GLB_UART_SIG_FUN_UART1_TXD);
}

/****************************************************************************/ /**
 * @brief  usb init
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
#if HAL_BOOT2_SUPPORT_USB_IAP
void hal_boot2_debug_usb_port_init(void)
{
}
#endif

/****************************************************************************/ /**
 * @brief  uart deinit
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_debug_uart_gpio_deinit(void)
{
    GLB_AHB_Slave1_Reset(BL_AHB_SLAVE1_UART0);
    GLB_AHB_Slave1_Reset(BL_AHB_SLAVE1_UART1);
    GLB_UART_Sig_Swap_Set(UART_SIG_SWAP_NONE);
}

/****************************************************************************/ /**
 * @brief  Check bootheader crc
 *
 * @param  data: bootheader data pointer
 *
 * @return boot_error_code type
 *
*******************************************************************************/
static uint32_t hal_boot_check_bootheader(struct hal_bootheader_t *header)
{
    uint32_t crc_pass = 0;
    uint32_t crc;

    if (header->bootCfg.bval.crcIgnore == 1 && header->crc32 == HAL_BOOT2_DEADBEEF_VAL) {
        //MSG("Crc ignored\r\n");
        crc_pass = 1;
    } else {
        crc = BFLB_Soft_CRC32((uint8_t *)header, sizeof(struct hal_bootheader_t) - sizeof(header->crc32));

        if (header->crc32 == crc) {
            crc_pass = 1;
        }
    }
    return crc_pass;
}

/****************************************************************************/ /**
 * @brief  Check if the input public key is the same as  burned in the efuse
 *
 * @param  g_boot_img_cfg: Boot image config pointer
 * @param  data: Image data pointer
 *
 * @return boot_error_code type
 *
*******************************************************************************/
int32_t hal_boot_parse_bootheader(boot2_image_config *boot_img_cfg, uint8_t *data)
{
    struct hal_bootheader_t *header = (struct hal_bootheader_t *)data;
    uint32_t crc_pass = 0;
    uint32_t i = 0;
    uint32_t *phash = (uint32_t *)header->hash;

    crc_pass = hal_boot_check_bootheader(header);

    if (!crc_pass) {
        //MSG_ERR("bootheader crc error\r\n");
        //blsp_dump_data((uint8_t *)&crc, 4);
        return 0x0204;
    }

    if (header->bootCfg.bval.notLoadInBoot) {
        return 0x0202;
    }

    /* Get which CPU's img it is*/
    for (i = 0; i < HAL_BOOT2_CPU_MAX; i++) {
        if (0 == memcmp((void *)&header->magicCode, HAL_BOOT2_CPU0_MAGIC,
                        sizeof(header->magicCode))) {
            break;
        } else if (0 == memcmp((void *)&header->magicCode, HAL_BOOT2_CPU1_MAGIC,
                               sizeof(header->magicCode))) {
            break;
        }
    }

    if (i == HAL_BOOT2_CPU_MAX) {
        /* No cpu img magic match */
        //MSG_ERR("Magic code error\r\n");
        return 0x0203;
    }

    if (boot_img_cfg == NULL) {
        return 0;
    }

    boot_img_cfg->pk_src = i;
    boot_img_cfg->img_valid = 0;

    /* Deal with pll config */

    /* Encrypt and sign */
    boot_img_cfg->basic_cfg.encrypt_type = header->bootCfg.bval.encrypt_type;
    boot_img_cfg->basic_cfg.sign_type = header->bootCfg.bval.sign;
    boot_img_cfg->basic_cfg.key_sel = header->bootCfg.bval.key_sel;

    /* Xip relative */
    boot_img_cfg->basic_cfg.no_segment = header->bootCfg.bval.no_segment;
    boot_img_cfg->cpu_cfg[0].cache_enable = header->bootCfg.bval.cache_enable;
    boot_img_cfg->basic_cfg.aes_region_lock = header->bootCfg.bval.aes_region_lock;
    //boot_img_cfg->cpu_cfg[1].halt_cpu = header->bootCfg.bval.halt_cpu1;
    boot_img_cfg->cpu_cfg[0].cache_way_dis = header->bootCfg.bval.cache_way_disable;
    boot_img_cfg->basic_cfg.hash_ignore = header->bootCfg.bval.hash_ignore;
    /* Firmware len*/
    boot_img_cfg->basic_cfg.img_len_cnt = header->img_segment_info.img_len;

    /* Boot entry and flash offset */
    boot_img_cfg->cpu_cfg[0].boot_entry = header->bootEntry;
    boot_img_cfg->basic_cfg.group_image_offset = header->img_start.flash_offset;

    boot_img_cfg->cpu_cfg[0].config_enable = 1;
    boot_img_cfg->cpu_cfg[0].halt_cpu = 0;

    //MSG("sign %d,encrypt:%d\r\n", boot_img_cfg->sign_type,boot_img_cfg->encrypt_type);

    /* Check encrypt and sign match*/
    if (g_efuse_cfg.encrypted[i] != 0) {
        if (boot_img_cfg->basic_cfg.encrypt_type == 0) {
            //MSG_ERR("Encrypt not fit\r\n");
            return 0x0205;
        }
    }

    if (g_efuse_cfg.sign[i] != boot_img_cfg->basic_cfg.sign_type) {
        //MSG_ERR("sign not fit\r\n");
        boot_img_cfg->basic_cfg.sign_type = g_efuse_cfg.sign[i];
        return 0x0206;
    }

    if (g_ps_mode == 1 && (!g_efuse_cfg.hbn_check_sign)) {
        /* In HBN Mode, if user select to ignore hash and sign*/
        boot_img_cfg->basic_cfg.hash_ignore = 1;
    } else if ((boot_img_cfg->basic_cfg.hash_ignore == 1 && *phash != HAL_BOOT2_DEADBEEF_VAL) ||
               g_efuse_cfg.sign[i] != 0) {
        /* If signed or user not really want to ignore, hash can't be ignored*/
        boot_img_cfg->basic_cfg.hash_ignore = 0;
    }

    if (g_user_hash_ignored) {
        boot_img_cfg->basic_cfg.hash_ignore = 1;
    }

    arch_memcpy_fast(boot_img_cfg->basic_cfg.hash, header->hash, sizeof(header->hash));

    if (boot_img_cfg->basic_cfg.img_len_cnt == 0) {
        return 0x0207;
    }

    return 0;
}

/****************************************************************************/ /**
 * @brief  clean cache 
 *
 * @param  
 *
 * @return cache set result
 *
*******************************************************************************/
void ATTR_TCM_SECTION hal_boot2_clean_cache(void)
{
    /* no need clean again since hal_boot2_set_cache will also clean
      unused way,and bl602 no data cache */
}

/****************************************************************************/ /**
 * @brief  set cache 
 *
 * @param  cont_read: continue read mode
 *         boot_img_cfg: cache set param
 *
 * @return cache set result
 *
*******************************************************************************/
BL_Err_Type ATTR_TCM_SECTION hal_boot2_set_cache(uint8_t cont_read, boot2_image_config *boot_img_cfg)
{
    return bflb_flash_set_cache(cont_read, boot_img_cfg->cpu_cfg[0].cache_enable,
                                boot_img_cfg->cpu_cfg[0].cache_way_dis,
                                boot_img_cfg->basic_cfg.group_image_offset);
}

/****************************************************************************/ /**
 * @brief  get the ram image name and count
 *
 * @param  img_name: ram image name in partition
 * @param  ram_img_cnt: ram image count that support boot from flash
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_get_ram_img_cnt(char *img_name[], uint32_t *ram_img_cnt)
{
    *ram_img_cnt = 0;
}

/****************************************************************************/ /**
 * @brief  get the ram image info
 *
 * @param  data: bootheader information
 * @param  image_offset: ram image offset in flash(from of bootheader)
 * @param  img_len: ram image length
 * @param  hash: pointer to hash pointer
 *
 * @return None
 *
*******************************************************************************/
void hal_boot2_get_img_info(uint8_t *data, uint32_t *image_offset, uint32_t *img_len, uint8_t **hash)
{
    *img_len = 0;
}

/****************************************************************************/ /**
 * @brief  release other cpu to boot up
 *
 * @param  core: core number
 * @param  boot_addr: boot address
 *
 * @return None
 *
*******************************************************************************/
void ATTR_TCM_SECTION hal_boot2_release_cpu(uint32_t core, uint32_t boot_addr)
{
}

/****************************************************************************/ /**
 * @brief  get xip address according to flash addr
 *
 * @param  flash_addr: flash address
 *
 * @return XIP Address
 *
*******************************************************************************/
uint32_t hal_boot2_get_xip_addr(uint32_t flash_addr)
{
    uint32_t img_offset = bflb_sf_ctrl_get_flash_image_offset(0,0);
    if (flash_addr >= img_offset) {
        return BL602_FLASH_XIP_BASE + (flash_addr - img_offset);
    } else {
        return 0;
    }
}

/****************************************************************************/ /**
 * @brief  get multi-group count
 *
 * @param  None
 *
 * @return 1 for multi-group 0 for not
 *
*******************************************************************************/
uint32_t hal_boot2_get_grp_count(void)
{
    return 1;
}

/****************************************************************************/ /**
 * @brief  get cpu count
 *
 * @param  None
 *
 * @return 1 for multi-group 0 for not
 *
*******************************************************************************/
uint32_t hal_boot2_get_cpu_count(void)
{
    return 1;
}

/****************************************************************************/ /**
 * @brief  get cpu count
 *
 * @param  None
 *
 * @return 1 for multi-group 0 for not
 *
*******************************************************************************/
uint32_t ATTR_TCM_SECTION hal_boot2_get_feature_flag(void)
{
    return HAL_BOOT2_SP_FLAG;
}

/****************************************************************************/ /**
 * @brief  get boot header offset
 *
 * @param  None
 *
 * @return bootheader offset
 *
*******************************************************************************/
uint32_t hal_boot2_get_bootheader_offset(void)
{
    return 0x00;
}
