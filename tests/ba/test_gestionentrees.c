#include "unity.h"
#include "pic16f946_stub.h"
#include <string.h>

#include "constantes.h"
#include "struct.h"

/* --- Define globals declared extern in global.h --- */

unsigned char uc_CompteurCycle;
unsigned char uc_CompteurAcq;
unsigned char uc_CompteurBase1S;
unsigned char b1_BT_1S;
unsigned char uc_CompteurLED;
unsigned char uc_CompteurBiStable;
unsigned char uc_FlagITTimer;

unsigned char uc_NB_ES;
unsigned char uc_ENTREE_TOR_NB;
unsigned char uc_RM_NB;
unsigned char uc_ENTREE_VARIATEUR_NB;
unsigned char uc_VARIATEUR_NB;
unsigned char uc_ENTREE_VOLET_NB;
unsigned char uc_RB_NB;
unsigned char uc_VOLET_NB;

struct struct_Entree st_Entrees[NB_ES];
unsigned short us_EntreesTOR;
struct struct_EntreeVariateur st_EntreesVariateur[ENTREE_VARIATEUR_NB];
struct struct_EntreeVoletOuStore st_EntreesVoletRoulant[ENTREE_VOLET_NB];

unsigned short us_SortiesRelais;
struct struct_SortieRelaisBistable st_SortiesRelaisBistables[RB_NB];
struct struct_Variateur st_Variateurs[VARIATEUR_NB];
unsigned char uc_CdeRelaisBistables[RB_NB];
unsigned short us_CdeVariateurs[VARIATEUR_NB];

unsigned char s_uc_PortA, s_uc_PortB, s_uc_PortC;
unsigned char s_uc_PortD, s_uc_PortE, s_uc_PortF;

union struct_Cde_BP st_Commandes;
unsigned char uc_compteur_ecriture_eeprom;
unsigned char uc_compteur_lecture_eeprom;

/* --- Controllable stub for uc_EtatEntree (replaces hard.c) --- */

static unsigned char stub_entree_values[NB_ES];

unsigned char uc_EtatEntree(unsigned char uc_NumeroEntree) {
    if (uc_NumeroEntree < NB_ES)
        return stub_entree_values[uc_NumeroEntree];
    return 0;
}

void vd_InitMicro(void) {}
void vd_ConfigBornier(void) {}

void vd_PiloterSortie(unsigned char t, unsigned char n, unsigned char e) {
    (void)t; (void)n; (void)e;
}

void vd_PosSortieBistable_ON(unsigned char n)  { (void)n; }
void vd_PosSortieBistable_OFF(unsigned char n) { (void)n; }
void vd_PiloteSortieVariateur(unsigned char n, unsigned char e) { (void)n; (void)e; }

#include "gestionentrees.h"

static void reset_all(void) {
    memset(st_Entrees, 0, sizeof(st_Entrees));
    memset(stub_entree_values, 0, sizeof(stub_entree_values));
    us_EntreesTOR = 0;
    memset(st_EntreesVariateur, 0, sizeof(st_EntreesVariateur));
    memset(st_EntreesVoletRoulant, 0, sizeof(st_EntreesVoletRoulant));
    uc_NB_ES = 21;
    uc_ENTREE_TOR_NB = 7;
    uc_ENTREE_VARIATEUR_NB = 4;
    uc_ENTREE_VOLET_NB = 10;
}

void setUp(void) { reset_all(); }
void tearDown(void) {}

/* --- Test: initial state after reset --- */
void test_initial_state_after_reset(void) {
    for (unsigned char i = 0; i < NB_ES; i++) {
        TEST_ASSERT_EQUAL_UINT8(0, st_Entrees[i].ub1_EtatActuel);
        TEST_ASSERT_EQUAL_UINT8(0, st_Entrees[i].ub4_AntiRebond_TempoChgtEtat);
        TEST_ASSERT_EQUAL_UINT8(0, st_Entrees[i].ub1_EtatFiltrePourApp);
        TEST_ASSERT_EQUAL_UINT8(0, st_Entrees[i].ub1_EtatFiltrePourAppPrecedent);
    }
    TEST_ASSERT_EQUAL_UINT16(0, us_EntreesTOR);
}

/* --- Test: debounce filters out bouncing --- */
void test_debounce_rejects_bouncing(void) {
    /* Alternate input 0 between 0 and 1 each cycle: should never validate */
    for (int cycle = 0; cycle < 20; cycle++) {
        stub_entree_values[0] = (cycle % 2) ? 1 : 0;
        vd_AcquisitionEntrees();
    }
    TEST_ASSERT_EQUAL_UINT8(0, st_Entrees[0].ub1_EtatFiltrePourApp);
}

/* --- Test: debounce validates stable input --- */
void test_debounce_validates_stable_input(void) {
    /* Set input 0 high and call acquisition enough times to pass debounce */
    stub_entree_values[0] = 1;
    for (int i = 0; i <= uc_ENTREE_ANTI_REBOND_CHGT_ETAT + 1; i++) {
        vd_AcquisitionEntrees();
    }
    TEST_ASSERT_EQUAL_UINT8(1, st_Entrees[0].ub1_EtatFiltrePourApp);
}

/* --- Test: debounce counter resets on state change --- */
void test_debounce_counter_resets_on_change(void) {
    stub_entree_values[0] = 1;
    /* Almost reach debounce threshold */
    for (int i = 0; i < uc_ENTREE_ANTI_REBOND_CHGT_ETAT - 1; i++) {
        vd_AcquisitionEntrees();
    }
    TEST_ASSERT_EQUAL_UINT8(0, st_Entrees[0].ub1_EtatFiltrePourApp);

    /* Bounce: change state before reaching threshold */
    stub_entree_values[0] = 0;
    vd_AcquisitionEntrees();
    TEST_ASSERT_EQUAL_UINT8(0, st_Entrees[0].ub4_AntiRebond_TempoChgtEtat);

    /* Go high again and wait full debounce */
    stub_entree_values[0] = 1;
    for (int i = 0; i <= uc_ENTREE_ANTI_REBOND_CHGT_ETAT + 1; i++) {
        vd_AcquisitionEntrees();
    }
    TEST_ASSERT_EQUAL_UINT8(1, st_Entrees[0].ub1_EtatFiltrePourApp);
}

/* --- Test: long press detection for variateur entries --- */

/* Resolve actual physical entry for variateur index (depends on board config) */
extern unsigned char uc_EntreeVariateurEntreeCarte(unsigned char uc_NumeroEntreeVariateur);

void test_variateur_long_press(void) {
    unsigned char entree_var0 = uc_EntreeVariateurEntreeCarte(uc_ENTREE_VARIATEUR0);
    st_EntreesVariateur[0].ub3_EntreeMode = uc_MODE_VARIATEUR;

    /* Simulate sustained press: filtered input goes high */
    st_Entrees[entree_var0].ub1_EtatFiltrePourApp = 1;
    st_Entrees[entree_var0].ub1_EtatFiltrePourAppPrecedent = 0;

    /* First call: detects rising edge, sets precedent */
    vd_AnalyserEntrees();

    /* Sustain press until long-press threshold */
    for (int i = 0; i < uc_TEMPS_VARIATEUR_APPUI_COURT_LONG + 1; i++) {
        vd_AnalyserEntrees();
    }
    TEST_ASSERT_EQUAL_UINT8(uc_VARIATEUR_APPUI_LONG,
                            st_EntreesVariateur[0].ub3_EtatCommande);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_state_after_reset);
    RUN_TEST(test_debounce_rejects_bouncing);
    RUN_TEST(test_debounce_validates_stable_input);
    RUN_TEST(test_debounce_counter_resets_on_change);
    RUN_TEST(test_variateur_long_press);
    return UNITY_END();
}
