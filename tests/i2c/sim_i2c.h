/*
 * sim_i2c.h -- Bus I2C et horloge simules pour les tests hote du pilote
 * I2C a attentes bornees du BP (MCF52xx, mode maitre polled).
 *
 * Feature : i2c-surge-protection-2026-10-007
 * Refs    : essensys-hub/essensys-board-SC944D#1
 *
 * Le simulateur avance une horloge virtuelle a chaque lecture de l'heure
 * (now_us) et fait evoluer les registres du controleur MCF52xx en
 * consequence : un octet commence quand le pilote acquitte IIF et se termine
 * apres le temps d'un octet a 50 kHz plus l'etirement d'horloge de
 * l'esclave. Les defauts (ligne collee, esclave absent, etirement) sont
 * injectes par le test. Le niveau des broches en GPIO (recuperation) est
 * modelise separement.
 */
#ifndef SIM_I2C_H
#define SIM_I2C_H

#include <stdint.h>
#include "mqx.h"
#include "bsp.h"
#include "io_prv.h"
#include "i2c.h"
#include "i2c_pol_prv.h"
#include "i2c_mcf52xx_prv.h"
#include "i2c_bounded.h"

#define SIM_BYTE_US       180u   /* 9 bits a 50 kHz */
#define SIM_TICK_US         2u   /* avance de l'horloge a chaque lecture */

typedef enum {
    SIM_FAULT_NONE = 0,
    SIM_FAULT_SDA_LOW,       /* SDA forcee basse en permanence           */
    SIM_FAULT_SCL_LOW,       /* SCL forcee basse en permanence           */
    SIM_FAULT_SLAVE_MID_BYTE /* esclave bloque au milieu d'un octet : SDA basse
                                jusqu'a un certain nombre de coups d'horloge */
} sim_fault_t;

typedef struct {
    /* Configuration injectee par le test */
    sim_fault_t fault;
    uint8_t     present_addr;     /* adresse 7 bits qui acquitte (0 = aucune) */
    uint32_t    stretch_us;       /* etirement d'horloge par octet            */
    uint8_t     rx_bytes[16];     /* octets renvoyes par l'esclave en lecture */
    unsigned    rx_len;
    unsigned    mid_byte_clocks;  /* coups d'horloge pour liberer SDA         */

    /* Etat */
    uint32_t    now_us;
    int         byte_busy;
    uint32_t    byte_done_at;
    int         next_is_address;
    int         reading;
    unsigned    rx_idx;
    uint8_t     tx_log[32];
    unsigned    tx_log_len;
    int         addressed_ok;

    /* GPIO (recuperation) */
    int         gpio_mode;
    int         scl_driven_low;
    int         sda_driven_low;
    unsigned    clock_pulses;
    int         stop_seen;
    uint8_t     i2cr_writes[8];
    unsigned    i2cr_write_count;
} sim_t;

extern sim_t sim;
extern volatile MCF52XX_I2C_STRUCT sim_regs;
extern const i2cb_hal_t sim_hal;

void sim_reset(void);
void sim_advance_us(uint32_t us);

/* Pilote de bout en bout : sequence de sl_fct_write_polled (099-37). */
typedef struct {
    IO_I2C_POLLED_DEVICE_STRUCT dev;
    MCF52XX_I2C_INFO_STRUCT     info;
} sim_drv_t;

void sim_drv_open(sim_drv_t *d);

/* Ecrit tx[0..txlen) a addr puis relit rxlen octets (repeated start).
 * Retourne 0 si tout est OK, sinon le numero de l'etape en erreur.
 * *elapsed_us recoit la duree de la transaction (horloge simulee). */
int sim_transaction(sim_drv_t *d, uint8_t addr, const uint8_t *tx, unsigned txlen,
                    uint8_t *rx, unsigned rxlen, uint32_t *elapsed_us);

/* Fonctions du pilote (bp/drivers/i2c/i2c_pol_mcf52xx.c). */
extern uint32_t _mcf52xx_i2c_polled_rx (IO_I2C_POLLED_DEVICE_STRUCT_PTR, unsigned char *, uint32_t);
extern uint32_t _mcf52xx_i2c_polled_tx (IO_I2C_POLLED_DEVICE_STRUCT_PTR, unsigned char *, uint32_t);
extern uint32_t _mcf52xx_i2c_polled_ioctl (VMCF52XX_I2C_INFO_STRUCT_PTR, uint32_t, uint32_t *);

#endif
