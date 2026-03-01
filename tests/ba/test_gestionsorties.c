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

/* --- Track calls to vd_PiloterSortie (stub replacing hard.c) --- */

#define MAX_CALLS 64
static struct {
    unsigned char type;
    unsigned char num;
    unsigned char etat;
} piloter_calls[MAX_CALLS];
static int piloter_call_count;

void vd_PiloterSortie(unsigned char uc_TypeSortie,
                       unsigned char uc_NumeroSortie,
                       unsigned char uc_Etat) {
    if (piloter_call_count < MAX_CALLS) {
        piloter_calls[piloter_call_count].type  = uc_TypeSortie;
        piloter_calls[piloter_call_count].num   = uc_NumeroSortie;
        piloter_calls[piloter_call_count].etat  = uc_Etat;
        piloter_call_count++;
    }
}

void vd_InitMicro(void) {}
void vd_ConfigBornier(void) {}
unsigned char uc_EtatEntree(unsigned char n) { (void)n; return 0; }
void vd_PosSortieBistable_ON(unsigned char n)  { (void)n; }
void vd_PosSortieBistable_OFF(unsigned char n) { (void)n; }
void vd_PiloteSortieVariateur(unsigned char n, unsigned char e) { (void)n; (void)e; }

#include "gestionsorties.h"

/* Internal functions from gestionsorties.c (not in header) */
extern void vd_PiloterLed(void);
extern void vd_PiloterRelais(void);
extern void vd_PiloterRelaisBistable(unsigned char uc_NumeroRelaisBistable);
extern void vd_PiloterRelaisBistables(void);

static void reset_all(void) {
    piloter_call_count = 0;
    memset(piloter_calls, 0, sizeof(piloter_calls));
    us_SortiesRelais = 0;
    st_Commandes.o = 0;
    uc_CompteurLED = 0;
    uc_CompteurBiStable = 0;
    uc_RM_NB = 7;
    uc_RB_NB = 10;
    uc_VARIATEUR_NB = 4;
    uc_VOLET_NB = uc_RB_NB >> 1;
    memset(st_SortiesRelaisBistables, 0, sizeof(st_SortiesRelaisBistables));
    memset(us_CdeVariateurs, 0, sizeof(us_CdeVariateurs));
}

void setUp(void) { reset_all(); }
void tearDown(void) {}

/* --- Test: relay activation via us_SortiesRelais --- */
void test_relay_activation(void) {
    us_SortiesRelais = 0x0005;  /* RM0 and RM2 ON */
    st_Commandes.ub1.AllumeTout = 0;

    vd_PiloterRelais();

    int rm0_on = 0, rm1_off = 0, rm2_on = 0;
    for (int i = 0; i < piloter_call_count; i++) {
        if (piloter_calls[i].type == uc_SORTIE_RELAIS) {
            if (piloter_calls[i].num == uc_RM0)
                rm0_on = (piloter_calls[i].etat == uc_ON);
            if (piloter_calls[i].num == uc_RM1)
                rm1_off = (piloter_calls[i].etat == uc_OFF);
            if (piloter_calls[i].num == uc_RM2)
                rm2_on = (piloter_calls[i].etat == uc_ON);
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(rm0_on,  "RM0 should be ON");
    TEST_ASSERT_TRUE_MESSAGE(rm1_off, "RM1 should be OFF");
    TEST_ASSERT_TRUE_MESSAGE(rm2_on,  "RM2 should be ON");
}

/* --- Test: AllumeTout forces all relays ON --- */
void test_allume_tout_forces_on(void) {
    us_SortiesRelais = 0x0000;  /* all OFF nominally */
    st_Commandes.ub1.AllumeTout = 1;

    vd_PiloterRelais();

    for (int i = 0; i < piloter_call_count; i++) {
        if (piloter_calls[i].type == uc_SORTIE_RELAIS) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(uc_ON, piloter_calls[i].etat,
                "AllumeTout should force all relays ON");
        }
    }
}

/* --- Test: bistable relay toggle changes state --- */
void test_bistable_toggle(void) {
    st_SortiesRelaisBistables[0].ub1_EtatRelaisBistable = 0;
    st_SortiesRelaisBistables[0].ub1_EtatRelaisBistableSouhaite = 1;

    vd_PiloterRelaisBistables();

    TEST_ASSERT_EQUAL_UINT8(1, st_SortiesRelaisBistables[0].ub1_EtatRelaisBistable);
}

/* --- Test: initial state — LED counter starts at 0 --- */
void test_initial_led_counter(void) {
    TEST_ASSERT_EQUAL_UINT8(0, uc_CompteurLED);

    piloter_call_count = 0;
    vd_PiloterLed();

    int led_on = 0;
    for (int i = 0; i < piloter_call_count; i++) {
        if (piloter_calls[i].type == uc_SORTIE_LED && piloter_calls[i].etat == uc_ON)
            led_on = 1;
    }
    TEST_ASSERT_TRUE_MESSAGE(led_on, "LED should be ON at counter=0");
    TEST_ASSERT_EQUAL_UINT8(1, uc_CompteurLED);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_relay_activation);
    RUN_TEST(test_allume_tout_forces_on);
    RUN_TEST(test_bistable_toggle);
    RUN_TEST(test_initial_led_counter);
    return UNITY_END();
}
