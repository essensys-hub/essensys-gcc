/*
 * test_ba_resil.c -- Resilience BP <-> BA (taches 5.3 a 5.7), horloge simulee.
 *
 * Feature : i2c-surge-protection-2026-10-007
 * Refs    : essensys-hub/essensys-board-SC944D#1
 */
#include "unity.h"
#include "ba_resil.h"

#include <string.h>
#include <stdlib.h>

static bar_ctx_t ctx;

#define MIN_MS  60000u
#define H_S     3600u

void setUp(void) { bar_init(&ctx); }
void tearDown(void) {}

static void cmd(unsigned ba, uint16_t s_on, uint16_t s_off, uint8_t v_on, uint8_t v_off,
                uint8_t up, uint8_t down, uint32_t now)
{
    uint8_t d[8];
    d[0] = (uint8_t)s_off; d[1] = (uint8_t)s_on;
    d[2] = (uint8_t)(s_off >> 8); d[3] = (uint8_t)(s_on >> 8);
    d[4] = v_off; d[5] = v_on; d[6] = up; d[7] = down;
    bar_cmd_merge(&ctx, ba, d, now);
}

/* ======================================================================== */
/* 5.3 -- Reinitialisation du canal I2C                                      */
/* ======================================================================== */
typedef struct {
    int live;           /* descripteurs vivants */
    int max_live;
    int opens;
    int close_fail_every;
    int closes;
} fake_io_t;

static void *f_open(void *u)
{
    fake_io_t *f = (fake_io_t *)u;
    f->opens++;
    f->live++;
    if (f->live > f->max_live) f->max_live = f->live;
    return malloc(16);
}

static int f_close(void *u, void *h)
{
    fake_io_t *f = (fake_io_t *)u;
    f->closes++;
    if (f->close_fail_every && (f->closes % f->close_fail_every) == 0) {
        return -1;   /* fclose refuse : le descripteur reste alloue */
    }
    f->live--;
    free(h);
    return 0;
}

static int f_configure(void *u, void *h) { (void)u; return h != NULL ? 0 : -1; }

/* NR: NR-gcc-4 essensys-hub/essensys-board-SC944D#1
 * 10 000 reinitialisations, dont une fermeture refusee sur trois : jamais
 * plus d'un descripteur vivant (aucune fuite) et le descripteur retourne
 * est toujours celui en service (pas de pointeur perime). */
static void test_NR_gcc_4_reinit_10000_no_leak_no_stale_pointer(void)
{
    fake_io_t f;
    bar_i2c_ops_t ops;
    void *fd;
    int ok, i;

    memset(&f, 0, sizeof(f));
    f.close_fail_every = 3;
    ops.open = f_open; ops.close = f_close; ops.configure = f_configure; ops.user = &f;

    fd = f_open(&f);
    for (i = 0; i < 10000; i++) {
        void *prev = fd;
        fd = bar_i2c_reopen(&ops, fd, &ok);
        TEST_ASSERT_NOT_NULL(fd);
        TEST_ASSERT_TRUE(ok);
        TEST_ASSERT_EQUAL_INT(1, f.live);
        if ((f.closes % 3) == 0) {
            TEST_ASSERT_EQUAL_PTR(prev, fd);   /* fermeture refusee : meme descripteur */
        }
    }
    TEST_ASSERT_EQUAL_INT(1, f.max_live);
    f_close(&f, fd);
}

/* ======================================================================== */
/* 5.4 -- Commandes conservees jusqu'a acquittement                          */
/* ======================================================================== */

/* NR: NR-gcc-5 essensys-hub/essensys-board-SC944D#1
 * Commande emise pendant l'absence du BA : jamais acquittee a tort, appliquee
 * au retour, quel que soit son age (etat maintenu). */
static void test_NR_gcc_5_command_kept_until_acked(void)
{
    uint8_t out[8];
    uint32_t t = 0;
    int i;

    cmd(1, 0x0004, 0, 0, 0, 0, 0, t);           /* allumer la lampe 2 du BA 0x12 */
    /* 5 min d'absence : chaque essai echoue, la commande reste en attente */
    for (i = 0; i < 60; i++) {
        t += 5000u;
        TEST_ASSERT_TRUE(bar_cmd_build(&ctx, 1, out));
        (void)bar_on_exchange(&ctx, 1, 0, t);
        TEST_ASSERT_TRUE(bar_cmd_pending(&ctx, 1));
    }
    TEST_ASSERT_EQUAL(BAR_ABSENT, bar_health(&ctx, 1));
    /* 30 min plus tard : toujours en attente (etat maintenu, pas d'expiration) */
    t += 30u * MIN_MS;
    TEST_ASSERT_EQUAL_HEX8(0, bar_cmd_expire(&ctx, 1, t));
    TEST_ASSERT_TRUE(bar_cmd_build(&ctx, 1, out));
    TEST_ASSERT_EQUAL_HEX8(0x04, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[0]);
    /* Retour du BA : acquittement */
    (void)bar_on_exchange(&ctx, 1, 1, t);
    bar_cmd_acked(&ctx, 1);
    TEST_ASSERT_FALSE(bar_cmd_pending(&ctx, 1));
}

/* NR: NR-gcc-5 essensys-hub/essensys-board-SC944D#1
 * Marche, arret, marche pendant l'absence : seul l'etat final est emis. */
static void test_NR_gcc_5_only_final_state_is_sent(void)
{
    uint8_t out[8];

    cmd(0, 0x0101, 0, 0x01, 0, 0, 0, 0);       /* marche */
    cmd(0, 0, 0x0101, 0, 0x01, 0, 0, 1000);    /* arret  */
    cmd(0, 0x0101, 0, 0x01, 0, 0, 0, 2000);    /* marche */
    TEST_ASSERT_TRUE(bar_cmd_build(&ctx, 0, out));
    TEST_ASSERT_EQUAL_HEX8(0x00, out[0]);  /* eteint LSB */
    TEST_ASSERT_EQUAL_HEX8(0x01, out[1]);  /* allume LSB */
    TEST_ASSERT_EQUAL_HEX8(0x00, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x01, out[3]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[4]);
    TEST_ASSERT_EQUAL_HEX8(0x01, out[5]);
}

/* NR: NR-gcc-5 essensys-hub/essensys-board-SC944D#1
 * Extinction prioritaire dans une meme trame (comme le BA). */
static void test_NR_gcc_5_same_frame_off_wins(void)
{
    uint8_t out[8];
    cmd(2, 0x0010, 0x0010, 0, 0, 0, 0, 0);
    TEST_ASSERT_TRUE(bar_cmd_build(&ctx, 2, out));
    TEST_ASSERT_EQUAL_HEX8(0x10, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[1]);
}

/* NR: NR-gcc-5 essensys-hub/essensys-board-SC944D#1
 * Commande de volet non appliquee au-dela de 5 min : abandonnee, signalee
 * expiree, jamais appliquee au retour du BA. */
static void test_NR_gcc_5_volet_command_expires_after_5_min(void)
{
    uint8_t out[8];

    cmd(1, 0, 0, 0, 0, 0x02, 0, 0);            /* ouvrir le volet 2 */
    TEST_ASSERT_EQUAL_HEX8(0, bar_cmd_expire(&ctx, 1, 5u * MIN_MS - 1u));
    TEST_ASSERT_EQUAL_HEX8(0x02, bar_cmd_expire(&ctx, 1, 5u * MIN_MS));
    TEST_ASSERT_EQUAL_UINT32(1, ctx.ba[1].expired);
    TEST_ASSERT_EQUAL_HEX8(0x02, ctx.ba[1].last_expired_mask);
    TEST_ASSERT_FALSE(bar_cmd_build(&ctx, 1, out));   /* rien a emettre au retour */
}

/* NR: NR-gcc-5 essensys-hub/essensys-board-SC944D#1
 * Delai configurable ; une nouvelle commande relance le delai ; ouverture
 * prioritaire dans une meme trame ; un ordre oppose remplace le precedent. */
static void test_NR_gcc_5_volet_rules(void)
{
    uint8_t out[8];

    ctx.volet_expiry_ms = 60000u;
    cmd(0, 0, 0, 0, 0, 0x01, 0x01, 0);           /* ouvre + ferme : ouvre */
    TEST_ASSERT_TRUE(bar_cmd_build(&ctx, 0, out));
    TEST_ASSERT_EQUAL_HEX8(0x01, out[6]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[7]);
    cmd(0, 0, 0, 0, 0, 0, 0x01, 50000u);         /* puis ferme */
    TEST_ASSERT_EQUAL_HEX8(0, bar_cmd_expire(&ctx, 0, 100000u));   /* relance */
    TEST_ASSERT_TRUE(bar_cmd_build(&ctx, 0, out));
    TEST_ASSERT_EQUAL_HEX8(0x00, out[6]);
    TEST_ASSERT_EQUAL_HEX8(0x01, out[7]);
    TEST_ASSERT_EQUAL_HEX8(0x01, bar_cmd_expire(&ctx, 0, 110000u));
}

/* NR: NR-gcc-5 essensys-hub/essensys-board-SC944D#1
 * Un ordre arrive pendant que la trame est en vol n'est pas efface par
 * l'acquittement de cette trame. */
static void test_NR_gcc_5_ack_only_clears_inflight_bits(void)
{
    uint8_t out[8];

    cmd(0, 0x0001, 0, 0, 0, 0, 0, 0);
    TEST_ASSERT_TRUE(bar_cmd_build(&ctx, 0, out));
    cmd(0, 0, 0x0001, 0, 0, 0, 0, 10);           /* arret arrive pendant l'emission */
    bar_cmd_acked(&ctx, 0);
    TEST_ASSERT_TRUE(bar_cmd_pending(&ctx, 0));
    TEST_ASSERT_TRUE(bar_cmd_build(&ctx, 0, out));
    TEST_ASSERT_EQUAL_HEX8(0x01, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[1]);
}

/* ======================================================================== */
/* 5.5 -- Resynchronisation                                                  */
/* ======================================================================== */

/* NR: NR-gcc-6 essensys-hub/essensys-board-SC944D#1
 * Au demarrage du BP, les trois BA sont a resynchroniser. */
static void test_NR_gcc_6_resync_on_bp_boot(void)
{
    unsigned i;
    for (i = 0; i < BAR_NB_BA; i++) {
        TEST_ASSERT_TRUE(bar_take_resync(&ctx, i, 0));
        TEST_ASSERT_FALSE(bar_take_resync(&ctx, i, 0));
    }
}

/* NR: NR-gcc-6 essensys-hub/essensys-board-SC944D#1
 * Retour d'un BA a OK apres une absence : resynchronisation demandee. */
static void test_NR_gcc_6_resync_on_return_to_ok(void)
{
    (void)bar_take_resync(&ctx, 2, 0);
    TEST_ASSERT_EQUAL_INT(0, bar_on_exchange(&ctx, 2, 1, 0));
    TEST_ASSERT_FALSE(bar_take_resync(&ctx, 2, 0));
    (void)bar_on_exchange(&ctx, 2, 0, 1000);
    (void)bar_on_exchange(&ctx, 2, 0, 6000);
    (void)bar_on_exchange(&ctx, 2, 0, 11000);
    TEST_ASSERT_EQUAL(BAR_ABSENT, bar_health(&ctx, 2));
    TEST_ASSERT_EQUAL_INT(1, bar_on_exchange(&ctx, 2, 1, 16000));
    TEST_ASSERT_TRUE(bar_take_resync(&ctx, 2, 16000));
}

/* NR: NR-gcc-6 essensys-hub/essensys-board-SC944D#1
 * Drapeau de redemarrage dans le 6e octet d'un BA 1.8 : resynchronisation.
 * Un BA 1.7 renvoie 0x55 : ignore, aucun effet. */
static void test_NR_gcc_6_comm_stat_restart_flag(void)
{
    unsigned i;
    for (i = 0; i < BAR_NB_BA; i++) (void)bar_take_resync(&ctx, i, 0);
    (void)bar_on_exchange(&ctx, 0, 1, 0);

    TEST_ASSERT_EQUAL_HEX8(0, bar_on_comm_stat(&ctx, 0, 0x55));   /* BA 1.7 */
    TEST_ASSERT_FALSE(bar_take_resync(&ctx, 0, 0));
    TEST_ASSERT_EQUAL_HEX8(0, bar_on_comm_stat(&ctx, 0, 0xFF));
    TEST_ASSERT_EQUAL_HEX8(0x00, bar_on_comm_stat(&ctx, 0, 0xA0)); /* BA 1.8 sans evenement */
    TEST_ASSERT_FALSE(bar_take_resync(&ctx, 0, 0));
    TEST_ASSERT_EQUAL_HEX8(0x03, bar_on_comm_stat(&ctx, 0, 0xA3)); /* redemarrage WDT */
    TEST_ASSERT_TRUE(bar_take_resync(&ctx, 0, 0));
    TEST_ASSERT_EQUAL_UINT32(1, ctx.ba[0].restarts_reported);
}

/* ======================================================================== */
/* 5.6 -- Sante, presence, index 12                                          */
/* ======================================================================== */

/* Simule la tache BA : interrogation de presence quand elle est due. */
static void run_presence(uint32_t from, uint32_t to, const int present[BAR_NB_BA])
{
    uint32_t t;
    unsigned i;
    for (t = from; t < to; t += 100u) {
        for (i = 0; i < BAR_NB_BA; i++) {
            if (bar_presence_due(&ctx, i, t) && bar_may_send(&ctx, i, t)) {
                (void)bar_on_exchange(&ctx, i, present[i], t);
            }
        }
    }
}

/* NR: NR-gcc-7 essensys-hub/essensys-board-SC944D#1
 * BA deconnecte sans aucune commande : bit positionne en moins de 120 s ;
 * reconnecte : bit efface en moins de 120 s et BA resynchronise. */
static void test_NR_gcc_7_bit_set_and_cleared_without_commands(void)
{
    int all[BAR_NB_BA] = { 1, 1, 1 };
    int one_out[BAR_NB_BA] = { 1, 0, 1 };
    uint32_t t, set_at = 0, clear_at = 0;
    unsigned i;

    for (i = 0; i < BAR_NB_BA; i++) (void)bar_take_resync(&ctx, i, 0);
    run_presence(0, 100000u, all);
    TEST_ASSERT_EQUAL_HEX8(0x00, bar_idx12_bits(&ctx));

    for (t = 100000u; t < 400000u; t += 100u) {
        run_presence(t, t + 100u, one_out);
        if (!set_at && (bar_idx12_bits(&ctx) & 0x08u)) set_at = t;
    }
    TEST_ASSERT_NOT_EQUAL(0, set_at);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(120000u, set_at - 100000u);
    TEST_ASSERT_EQUAL_HEX8(0x08, bar_idx12_bits(&ctx));     /* seul le BA retire */

    for (t = 400000u; t < 700000u; t += 100u) {
        run_presence(t, t + 100u, all);
        if (!clear_at && !(bar_idx12_bits(&ctx) & 0x08u)) clear_at = t;
    }
    TEST_ASSERT_NOT_EQUAL(0, clear_at);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(120000u, clear_at - 400000u);
    TEST_ASSERT_TRUE(bar_take_resync(&ctx, 1, clear_at));
}

/* NR: NR-gcc-7 essensys-hub/essensys-board-SC944D#1
 * Interrogation de presence au moins toutes les 60 s pour un BA OK. */
static void test_NR_gcc_7_presence_poll_at_least_every_60s(void)
{
    (void)bar_on_exchange(&ctx, 0, 1, 0);
    TEST_ASSERT_FALSE(bar_presence_due(&ctx, 0, BAR_PRESENCE_POLL_OK_MS - 1u));
    TEST_ASSERT_TRUE(bar_presence_due(&ctx, 0, BAR_PRESENCE_POLL_OK_MS));
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(60000u, BAR_PRESENCE_POLL_OK_MS);
}

/* NR: NR-gcc-7 essensys-hub/essensys-board-SC944D#1
 * Codage de l'index 12 inchange : trois BA absents -> (idx & 0x1C) == 0x1C,
 * bits 0 et 1 independants de l'etat des BA. */
static void test_NR_gcc_7_index12_encoding_unchanged(void)
{
    unsigned i;
    int k;

    for (i = 0; i < BAR_NB_BA; i++) {
        for (k = 0; k < 3; k++) (void)bar_on_exchange(&ctx, i, 0, (uint32_t)k * 5000u);
    }
    TEST_ASSERT_EQUAL_HEX8(0x1C, bar_idx12_bits(&ctx));
    TEST_ASSERT_EQUAL_HEX8(0x1C, bar_idx12_apply(&ctx, 0x00));
    TEST_ASSERT_EQUAL_HEX8(0x1D, bar_idx12_apply(&ctx, 0x01));   /* bit 0 compteur garde */
    TEST_ASSERT_EQUAL_HEX8(0x1E, bar_idx12_apply(&ctx, 0x02));   /* bit 1 IHM garde */
    TEST_ASSERT_EQUAL_HEX8(0x1F, bar_idx12_apply(&ctx, 0x1F));
    TEST_ASSERT_EQUAL_HEX8(0x04, bar_idx12_bit(0));   /* 0x11 SC940D */
    TEST_ASSERT_EQUAL_HEX8(0x08, bar_idx12_bit(1));   /* 0x12 SC942C */
    TEST_ASSERT_EQUAL_HEX8(0x10, bar_idx12_bit(2));   /* 0x13 SC941C */
    TEST_ASSERT_EQUAL_UINT32(1, ctx.all_down_events);
    (void)bar_on_exchange(&ctx, 0, 1, 20000u);
    TEST_ASSERT_EQUAL_HEX8(0x01, bar_idx12_apply(&ctx, 0x1D) & 0x03);
    TEST_ASSERT_EQUAL_HEX8(0x18, bar_idx12_bits(&ctx));
}

/* NR: NR-gcc-7 essensys-hub/essensys-board-SC944D#1
 * Un BA non OK n'est pas martele : au plus un essai toutes les 5 s, ce qui
 * garantit un retour (et l'application des commandes) en moins de 10 s. */
static void test_NR_gcc_7_retry_pacing_when_absent(void)
{
    (void)bar_on_exchange(&ctx, 2, 0, 0);
    TEST_ASSERT_FALSE(bar_may_send(&ctx, 2, 4999u));
    TEST_ASSERT_TRUE(bar_may_send(&ctx, 2, 5000u));
    TEST_ASSERT_LESS_THAN_UINT32(10000u, BAR_RETRY_NOK_MS);
}

/* ======================================================================== */
/* 5.7 -- Cycle TE2 en dernier recours                                       */
/* ======================================================================== */
#define EPOCH0  1790000000u   /* date murale arbitraire */

/* Fait tourner la politique TE2 de t0 a t1 (pas de 1 s) avec un bus bloque
 * depuis blocked_since ; compte les debuts de cycle. */
static unsigned run_te2(uint32_t t0, uint32_t t1, int blocked, uint32_t blocked_since)
{
    uint32_t t;
    unsigned cuts = 0;
    for (t = t0; t < t1; t += 1000u) {
        bar_te2_action_t a = bar_te2_step(&ctx, t, EPOCH0 + t / 1000u, blocked, blocked_since, 0);
        if (a == BAR_TE2_CUT_START) cuts++;
    }
    return cuts;
}

/* NR: NR-gcc-8 essensys-hub/essensys-board-SC944D#1
 * Pas de cycle avant 60 s de blocage, un seul cycle ensuite (compte et
 * date), suivi d'une resynchronisation complete. */
static void test_NR_gcc_8_cycle_after_60s_once(void)
{
    unsigned i;

    bar_te2_set_j58_open(&ctx, 0);
    for (i = 0; i < BAR_NB_BA; i++) (void)bar_take_resync(&ctx, i, 0);
    TEST_ASSERT_EQUAL_UINT(0, run_te2(0, 59000u, 1, 0));
    TEST_ASSERT_EQUAL_UINT(1, run_te2(59000u, 70000u, 1, 0));
    TEST_ASSERT_EQUAL_UINT32(1, ctx.te2.total_cycles);
    TEST_ASSERT_EQUAL_UINT32(EPOCH0 + 60u, bar_te2_last_cycle_epoch(&ctx));
    TEST_ASSERT_TRUE(ctx.te2.nv_dirty);
    /* fin de coupure apres BAR_TE2_CUT_MS : resynchronisation de tous les BA */
    for (i = 0; i < BAR_NB_BA; i++) TEST_ASSERT_TRUE(ctx.ba[i].resync_pending);
}

/* NR: NR-gcc-8 essensys-hub/essensys-board-SC944D#1
 * Au moins 1 h entre deux cycles, meme si le bus se rebloque. */
static void test_NR_gcc_8_one_hour_spacing(void)
{
    bar_te2_set_j58_open(&ctx, 0);
    TEST_ASSERT_EQUAL_UINT(1, run_te2(0, 70000u, 1, 0));
    run_te2(70000u, 71000u, 0, 0);                         /* le bus revient */
    /* re-blocage a t = 100 s : 60 s plus tard, < 1 h depuis le cycle */
    TEST_ASSERT_EQUAL_UINT(0, run_te2(100000u, 3600000u, 1, 100000u));
    TEST_ASSERT_EQUAL(BAR_TE2_WHY_SPACING, ctx.te2.why);
    TEST_ASSERT_EQUAL_UINT(1, run_te2(3600000u, 3700000u, 1, 100000u));
}

/* NR: NR-gcc-9 essensys-hub/essensys-board-SC944D#1
 * Trois cycles au plus par 24 h glissantes, compteur conserve a travers un
 * redemarrage du BP (enregistrement NV). */
static void test_NR_gcc_9_three_per_24h_persisted_across_reboot(void)
{
    uint8_t nv[BAR_TE2_NV_SIZE];
    uint32_t t = 0;
    int k;

    bar_te2_set_j58_open(&ctx, 0);
    for (k = 0; k < 3; k++) {
        uint32_t blk = t;
        TEST_ASSERT_EQUAL_UINT(1, run_te2(t, t + 70000u, 1, blk));
        run_te2(t + 70000u, t + 71000u, 0, 0);            /* bus revenu */
        t += 2u * 3600000u;                                /* 2 h plus tard */
    }
    TEST_ASSERT_EQUAL_UINT32(3, ctx.te2.total_cycles);

    /* Redemarrage du BP : sauvegarde puis rechargement */
    bar_te2_save(&ctx, nv);
    bar_init(&ctx);
    TEST_ASSERT_EQUAL_INT(1, bar_te2_load(&ctx, nv));
    TEST_ASSERT_EQUAL_UINT32(3, ctx.te2.total_cycles);
    TEST_ASSERT_FALSE(ctx.te2.j58_open);

    /* 4e blocage dans les 24 h : aucun cycle */
    TEST_ASSERT_EQUAL_UINT(0, run_te2(t, t + 3600000u, 1, t));
    TEST_ASSERT_EQUAL(BAR_TE2_WHY_LIMIT, ctx.te2.why);
    /* Hors fenetre (24 h apres le 1er cycle) : de nouveau possible */
    TEST_ASSERT_EQUAL_UINT(1, run_te2(24u * 3600000u + 61000u, 24u * 3600000u + 120000u, 1, t));
}

/* NR: NR-gcc-9 essensys-hub/essensys-board-SC944D#1
 * Enregistrement NV corrompu ou absent : etat par defaut (J58 ouvert). */
static void test_NR_gcc_9_nv_corrupt_is_rejected(void)
{
    uint8_t nv[BAR_TE2_NV_SIZE];
    bar_te2_set_j58_open(&ctx, 0);
    bar_te2_save(&ctx, nv);
    nv[10] ^= 0x40;
    bar_init(&ctx);
    TEST_ASSERT_EQUAL_INT(0, bar_te2_load(&ctx, nv));
    TEST_ASSERT_TRUE(ctx.te2.j58_open);
    memset(nv, 0xFF, sizeof(nv));
    TEST_ASSERT_EQUAL_INT(0, bar_te2_load(&ctx, nv));
}

/* NR: NR-gcc-10 essensys-hub/essensys-board-SC944D#1
 * Cycle sans effet : plus aucun cycle jusqu'au retour du bus ou a une
 * remise a zero manuelle ; la suspension survit a un redemarrage. */
static void test_NR_gcc_10_no_new_cycle_after_ineffective_cycle(void)
{
    uint8_t nv[BAR_TE2_NV_SIZE];

    bar_te2_set_j58_open(&ctx, 0);
    TEST_ASSERT_EQUAL_UINT(1, run_te2(0, 70000u, 1, 0));
    /* le bus reste bloque (BA mort) pendant 10 h */
    TEST_ASSERT_EQUAL_UINT(0, run_te2(70000u, 36000000u, 1, 0));
    TEST_ASSERT_TRUE(ctx.te2.suspended);
    TEST_ASSERT_EQUAL(BAR_TE2_WHY_SUSPENDED, ctx.te2.why);

    bar_te2_save(&ctx, nv);
    bar_init(&ctx);
    TEST_ASSERT_EQUAL_INT(1, bar_te2_load(&ctx, nv));
    TEST_ASSERT_TRUE(ctx.te2.suspended);
    TEST_ASSERT_EQUAL_UINT(0, run_te2(36000000u, 37000000u, 1, 36000000u));

    /* remise a zero manuelle depuis la console */
    bar_te2_manual_reset(&ctx);
    TEST_ASSERT_EQUAL_UINT(1, run_te2(37000000u, 37100000u, 1, 36000000u));
}

/* NR: NR-gcc-10 essensys-hub/essensys-board-SC944D#1
 * Le retour du bus leve la suspension. */
static void test_NR_gcc_10_bus_return_clears_suspension(void)
{
    bar_te2_set_j58_open(&ctx, 0);
    TEST_ASSERT_EQUAL_UINT(1, run_te2(0, 70000u, 1, 0));
    run_te2(70000u, 200000u, 1, 0);
    TEST_ASSERT_TRUE(ctx.te2.suspended);
    run_te2(200000u, 201000u, 0, 0);
    TEST_ASSERT_FALSE(ctx.te2.suspended);
}

/* NR: NR-gcc-11 essensys-hub/essensys-board-SC944D#1
 * J58 declare ouvert (valeur par defaut) : ni cycle ni comptage, blocage
 * signale. En mode secouru : pas de cycle non plus. */
static void test_NR_gcc_11_j58_open_no_cycle_no_count(void)
{
    TEST_ASSERT_TRUE(ctx.te2.j58_open);
    TEST_ASSERT_EQUAL_UINT(0, run_te2(0, 3u * 3600000u, 1, 0));
    TEST_ASSERT_EQUAL_UINT32(0, ctx.te2.total_cycles);
    TEST_ASSERT_EQUAL(BAR_TE2_WHY_J58_OPEN, ctx.te2.why);
    TEST_ASSERT_GREATER_THAN_UINT32(0, ctx.te2.blocked_reports);

    bar_te2_set_j58_open(&ctx, 0);
    TEST_ASSERT_EQUAL(BAR_TE2_NONE, bar_te2_step(&ctx, 100000u, EPOCH0, 1, 0, 1));
    TEST_ASSERT_EQUAL(BAR_TE2_WHY_SECOURU, ctx.te2.why);
    /* heure murale inconnue : abstention prudente */
    TEST_ASSERT_EQUAL(BAR_TE2_NONE, bar_te2_step(&ctx, 100000u, 0u, 1, 0, 0));
    TEST_ASSERT_EQUAL(BAR_TE2_WHY_TIME_UNKNOWN, ctx.te2.why);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_NR_gcc_4_reinit_10000_no_leak_no_stale_pointer);
    RUN_TEST(test_NR_gcc_5_command_kept_until_acked);
    RUN_TEST(test_NR_gcc_5_only_final_state_is_sent);
    RUN_TEST(test_NR_gcc_5_same_frame_off_wins);
    RUN_TEST(test_NR_gcc_5_volet_command_expires_after_5_min);
    RUN_TEST(test_NR_gcc_5_volet_rules);
    RUN_TEST(test_NR_gcc_5_ack_only_clears_inflight_bits);
    RUN_TEST(test_NR_gcc_6_resync_on_bp_boot);
    RUN_TEST(test_NR_gcc_6_resync_on_return_to_ok);
    RUN_TEST(test_NR_gcc_6_comm_stat_restart_flag);
    RUN_TEST(test_NR_gcc_7_bit_set_and_cleared_without_commands);
    RUN_TEST(test_NR_gcc_7_presence_poll_at_least_every_60s);
    RUN_TEST(test_NR_gcc_7_index12_encoding_unchanged);
    RUN_TEST(test_NR_gcc_7_retry_pacing_when_absent);
    RUN_TEST(test_NR_gcc_8_cycle_after_60s_once);
    RUN_TEST(test_NR_gcc_8_one_hour_spacing);
    RUN_TEST(test_NR_gcc_9_three_per_24h_persisted_across_reboot);
    RUN_TEST(test_NR_gcc_9_nv_corrupt_is_rejected);
    RUN_TEST(test_NR_gcc_10_no_new_cycle_after_ineffective_cycle);
    RUN_TEST(test_NR_gcc_10_bus_return_clears_suspension);
    RUN_TEST(test_NR_gcc_11_j58_open_no_cycle_no_count);
    return UNITY_END();
}
