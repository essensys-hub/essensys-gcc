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

/* Stubs for functions declared in headers but defined elsewhere */
void vd_PiloterSorties(void) {}
void vd_ArretPilotageRelaisBist(void) {}
void vd_PilotageRelaisBist(void) {}

#include "variateur.h"

void setUp(void) {
    memset(st_Variateurs, 0, sizeof(st_Variateurs));
    memset(us_CdeVariateurs, 0, sizeof(us_CdeVariateurs));
}
void tearDown(void) {}

/* --- Test: rampe montante — encoding increases with value --- */
void test_ramp_up_encoding(void) {
    unsigned short prev = us_CodeVariateur(0);
    for (char val = 1; val <= 100; val++) {
        unsigned short cur = us_CodeVariateur(val);
        /* Each encoded value must be unique and non-zero */
        TEST_ASSERT_NOT_EQUAL_MESSAGE(0, cur, "Encoded value should not be zero");
        TEST_ASSERT_NOT_EQUAL_MESSAGE(prev, cur, "Each step should produce a different code");
        prev = cur;
    }
}

/* --- Test: min bound — value 0 produces valid serial frame --- */
void test_min_bound_encoding(void) {
    unsigned short code = us_CodeVariateur(0);
    /* Start bit = 0 (bit 0 of the shifted value), stop bits = 1 (bits 9-10) */
    TEST_ASSERT_BITS(0x0001, 0x0000, code);    /* bit 0 is start bit = 0 */
    TEST_ASSERT_BITS(0x0600, 0x0600, code);    /* bits 9-10 are stop bits = 1 */
}

/* --- Test: max bound — value 100 produces valid serial frame --- */
void test_max_bound_encoding(void) {
    unsigned short code = us_CodeVariateur(100);
    TEST_ASSERT_BITS(0x0001, 0x0000, code);    /* start bit = 0 */
    TEST_ASSERT_BITS(0x0600, 0x0600, code);    /* stop bits = 1 */
    TEST_ASSERT_NOT_EQUAL(0, code);
}

/* --- Test: extinction — value 0 encodes offset only --- */
void test_extinction_offset(void) {
    unsigned short code = us_CodeVariateur(0);
    /*
     * Expected: (0 + uc_CDE_VAR_OFFSET) << 1 masked to 0x01FE, then | 0x0600
     * uc_CDE_VAR_OFFSET = 10, so data = 10 << 1 = 20 = 0x0014
     * Final = 0x0014 | 0x0600 = 0x0614
     */
    TEST_ASSERT_EQUAL_UINT16(0x0614, code);
}

/* --- Test: full scale — value 100 encodes correctly --- */
void test_full_scale_encoding(void) {
    unsigned short code = us_CodeVariateur(100);
    /*
     * (100 + 10) << 1 = 110 << 1 = 220 = 0x00DC
     * Masked 0x01FE => 0x00DC, | 0x0600 => 0x06DC
     */
    TEST_ASSERT_EQUAL_UINT16(0x06DC, code);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ramp_up_encoding);
    RUN_TEST(test_min_bound_encoding);
    RUN_TEST(test_max_bound_encoding);
    RUN_TEST(test_extinction_offset);
    RUN_TEST(test_full_scale_encoding);
    return UNITY_END();
}
