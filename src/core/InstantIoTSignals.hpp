/*
 * InstantIoT — signal addresses.
 *
 * `I0`..`I255` are provided BY THE LIBRARY, like `A0` or `LED_BUILTIN`.
 * Nothing to generate, nothing to declare, nothing to export from the app:
 * the sketch writes `InstantIoT.write(I0, v)` and that is all.
 *
 * They are typed constants in a namespace, never `#define`, for two reasons
 * that both surface as clear compiler errors instead of silent nonsense:
 *
 *   - `InstantIoT.write(IO, x)` — letter O instead of zero — does not compile.
 *   - a user declaring their own `I1` gets a redefinition error, not an
 *     incomprehensible message about a string literal.
 *
 * The address is one byte on the wire; the board never sends its own identity
 * with it, because the connection is already authenticated by its token.
 */
#pragma once
#include <stdint.h>

namespace InstantIoTSignals {

/** A signal address. The wrapper is what makes a wrong token fail to compile. */
struct SignalRef {
    uint8_t addr;
    constexpr explicit SignalRef(uint8_t a) : addr(a) {}
};

}  // namespace InstantIoTSignals

using InstantIoTSignals::SignalRef;

constexpr SignalRef I0{0};
constexpr SignalRef I1{1};
constexpr SignalRef I2{2};
constexpr SignalRef I3{3};
constexpr SignalRef I4{4};
constexpr SignalRef I5{5};
constexpr SignalRef I6{6};
constexpr SignalRef I7{7};
constexpr SignalRef I8{8};
constexpr SignalRef I9{9};
constexpr SignalRef I10{10};
constexpr SignalRef I11{11};
constexpr SignalRef I12{12};
constexpr SignalRef I13{13};
constexpr SignalRef I14{14};
constexpr SignalRef I15{15};
constexpr SignalRef I16{16};
constexpr SignalRef I17{17};
constexpr SignalRef I18{18};
constexpr SignalRef I19{19};
constexpr SignalRef I20{20};
constexpr SignalRef I21{21};
constexpr SignalRef I22{22};
constexpr SignalRef I23{23};
constexpr SignalRef I24{24};
constexpr SignalRef I25{25};
constexpr SignalRef I26{26};
constexpr SignalRef I27{27};
constexpr SignalRef I28{28};
constexpr SignalRef I29{29};
constexpr SignalRef I30{30};
constexpr SignalRef I31{31};
constexpr SignalRef I32{32};
constexpr SignalRef I33{33};
constexpr SignalRef I34{34};
constexpr SignalRef I35{35};
constexpr SignalRef I36{36};
constexpr SignalRef I37{37};
constexpr SignalRef I38{38};
constexpr SignalRef I39{39};
constexpr SignalRef I40{40};
constexpr SignalRef I41{41};
constexpr SignalRef I42{42};
constexpr SignalRef I43{43};
constexpr SignalRef I44{44};
constexpr SignalRef I45{45};
constexpr SignalRef I46{46};
constexpr SignalRef I47{47};
constexpr SignalRef I48{48};
constexpr SignalRef I49{49};
constexpr SignalRef I50{50};
constexpr SignalRef I51{51};
constexpr SignalRef I52{52};
constexpr SignalRef I53{53};
constexpr SignalRef I54{54};
constexpr SignalRef I55{55};
constexpr SignalRef I56{56};
constexpr SignalRef I57{57};
constexpr SignalRef I58{58};
constexpr SignalRef I59{59};
constexpr SignalRef I60{60};
constexpr SignalRef I61{61};
constexpr SignalRef I62{62};
constexpr SignalRef I63{63};
constexpr SignalRef I64{64};
constexpr SignalRef I65{65};
constexpr SignalRef I66{66};
constexpr SignalRef I67{67};
constexpr SignalRef I68{68};
constexpr SignalRef I69{69};
constexpr SignalRef I70{70};
constexpr SignalRef I71{71};
constexpr SignalRef I72{72};
constexpr SignalRef I73{73};
constexpr SignalRef I74{74};
constexpr SignalRef I75{75};
constexpr SignalRef I76{76};
constexpr SignalRef I77{77};
constexpr SignalRef I78{78};
constexpr SignalRef I79{79};
constexpr SignalRef I80{80};
constexpr SignalRef I81{81};
constexpr SignalRef I82{82};
constexpr SignalRef I83{83};
constexpr SignalRef I84{84};
constexpr SignalRef I85{85};
constexpr SignalRef I86{86};
constexpr SignalRef I87{87};
constexpr SignalRef I88{88};
constexpr SignalRef I89{89};
constexpr SignalRef I90{90};
constexpr SignalRef I91{91};
constexpr SignalRef I92{92};
constexpr SignalRef I93{93};
constexpr SignalRef I94{94};
constexpr SignalRef I95{95};
constexpr SignalRef I96{96};
constexpr SignalRef I97{97};
constexpr SignalRef I98{98};
constexpr SignalRef I99{99};
constexpr SignalRef I100{100};
constexpr SignalRef I101{101};
constexpr SignalRef I102{102};
constexpr SignalRef I103{103};
constexpr SignalRef I104{104};
constexpr SignalRef I105{105};
constexpr SignalRef I106{106};
constexpr SignalRef I107{107};
constexpr SignalRef I108{108};
constexpr SignalRef I109{109};
constexpr SignalRef I110{110};
constexpr SignalRef I111{111};
constexpr SignalRef I112{112};
constexpr SignalRef I113{113};
constexpr SignalRef I114{114};
constexpr SignalRef I115{115};
constexpr SignalRef I116{116};
constexpr SignalRef I117{117};
constexpr SignalRef I118{118};
constexpr SignalRef I119{119};
constexpr SignalRef I120{120};
constexpr SignalRef I121{121};
constexpr SignalRef I122{122};
constexpr SignalRef I123{123};
constexpr SignalRef I124{124};
constexpr SignalRef I125{125};
constexpr SignalRef I126{126};
constexpr SignalRef I127{127};
constexpr SignalRef I128{128};
constexpr SignalRef I129{129};
constexpr SignalRef I130{130};
constexpr SignalRef I131{131};
constexpr SignalRef I132{132};
constexpr SignalRef I133{133};
constexpr SignalRef I134{134};
constexpr SignalRef I135{135};
constexpr SignalRef I136{136};
constexpr SignalRef I137{137};
constexpr SignalRef I138{138};
constexpr SignalRef I139{139};
constexpr SignalRef I140{140};
constexpr SignalRef I141{141};
constexpr SignalRef I142{142};
constexpr SignalRef I143{143};
constexpr SignalRef I144{144};
constexpr SignalRef I145{145};
constexpr SignalRef I146{146};
constexpr SignalRef I147{147};
constexpr SignalRef I148{148};
constexpr SignalRef I149{149};
constexpr SignalRef I150{150};
constexpr SignalRef I151{151};
constexpr SignalRef I152{152};
constexpr SignalRef I153{153};
constexpr SignalRef I154{154};
constexpr SignalRef I155{155};
constexpr SignalRef I156{156};
constexpr SignalRef I157{157};
constexpr SignalRef I158{158};
constexpr SignalRef I159{159};
constexpr SignalRef I160{160};
constexpr SignalRef I161{161};
constexpr SignalRef I162{162};
constexpr SignalRef I163{163};
constexpr SignalRef I164{164};
constexpr SignalRef I165{165};
constexpr SignalRef I166{166};
constexpr SignalRef I167{167};
constexpr SignalRef I168{168};
constexpr SignalRef I169{169};
constexpr SignalRef I170{170};
constexpr SignalRef I171{171};
constexpr SignalRef I172{172};
constexpr SignalRef I173{173};
constexpr SignalRef I174{174};
constexpr SignalRef I175{175};
constexpr SignalRef I176{176};
constexpr SignalRef I177{177};
constexpr SignalRef I178{178};
constexpr SignalRef I179{179};
constexpr SignalRef I180{180};
constexpr SignalRef I181{181};
constexpr SignalRef I182{182};
constexpr SignalRef I183{183};
constexpr SignalRef I184{184};
constexpr SignalRef I185{185};
constexpr SignalRef I186{186};
constexpr SignalRef I187{187};
constexpr SignalRef I188{188};
constexpr SignalRef I189{189};
constexpr SignalRef I190{190};
constexpr SignalRef I191{191};
constexpr SignalRef I192{192};
constexpr SignalRef I193{193};
constexpr SignalRef I194{194};
constexpr SignalRef I195{195};
constexpr SignalRef I196{196};
constexpr SignalRef I197{197};
constexpr SignalRef I198{198};
constexpr SignalRef I199{199};
constexpr SignalRef I200{200};
constexpr SignalRef I201{201};
constexpr SignalRef I202{202};
constexpr SignalRef I203{203};
constexpr SignalRef I204{204};
constexpr SignalRef I205{205};
constexpr SignalRef I206{206};
constexpr SignalRef I207{207};
constexpr SignalRef I208{208};
constexpr SignalRef I209{209};
constexpr SignalRef I210{210};
constexpr SignalRef I211{211};
constexpr SignalRef I212{212};
constexpr SignalRef I213{213};
constexpr SignalRef I214{214};
constexpr SignalRef I215{215};
constexpr SignalRef I216{216};
constexpr SignalRef I217{217};
constexpr SignalRef I218{218};
constexpr SignalRef I219{219};
constexpr SignalRef I220{220};
constexpr SignalRef I221{221};
constexpr SignalRef I222{222};
constexpr SignalRef I223{223};
constexpr SignalRef I224{224};
constexpr SignalRef I225{225};
constexpr SignalRef I226{226};
constexpr SignalRef I227{227};
constexpr SignalRef I228{228};
constexpr SignalRef I229{229};
constexpr SignalRef I230{230};
constexpr SignalRef I231{231};
constexpr SignalRef I232{232};
constexpr SignalRef I233{233};
constexpr SignalRef I234{234};
constexpr SignalRef I235{235};
constexpr SignalRef I236{236};
constexpr SignalRef I237{237};
constexpr SignalRef I238{238};
constexpr SignalRef I239{239};
constexpr SignalRef I240{240};
constexpr SignalRef I241{241};
constexpr SignalRef I242{242};
constexpr SignalRef I243{243};
constexpr SignalRef I244{244};
constexpr SignalRef I245{245};
constexpr SignalRef I246{246};
constexpr SignalRef I247{247};
constexpr SignalRef I248{248};
constexpr SignalRef I249{249};
constexpr SignalRef I250{250};
constexpr SignalRef I251{251};
constexpr SignalRef I252{252};
constexpr SignalRef I253{253};
constexpr SignalRef I254{254};
constexpr SignalRef I255{255};
