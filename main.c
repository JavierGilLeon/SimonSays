#include <msp430.h> 
#include "grlib.h"
#include "uart_STDIO.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_MSP430G2_Crystalfontz128x128_ST7735.h"
#include <stdio.h> //Incluimos librerias para poder usar la pantalla del BoosterPack


//-------------------------- MACROS -----------------------------------------------------------
#define MODULE 4
#define TAM_BARRA 90
#define NUM_RONDAS 32
#define FEEDBACK 0xB8 // POLINOMIO x^8 + x^6 + x^5 + x^4 + 1 = 10111000
#define TIEMPO_ILUMINADO 50

#define DO  30578
#define MI  24270
#define SOL 20408
#define SI  16202

//MACROS UTILIZADAS PARA DEPURACION
#define CALCULA_DISTRIBUCION 0 // Utilizada en la funcion GeneraSecuencia
#define LIMPIAFLASH 0          // Utilizada antes de comprobar la contrasena de la FLASH
#define PASARONDA 0            // Utilizada al final del estado de RondaJugador

//--------------------------------------------------------------------------------------------


//-------- DECLARACIONES DE FUNCIONES -----------------
void GeneraSecuencia(void);
void PintaRect(char Valor);
void PintaFlecha(char Valor);
int lee_ch(char canal);
void TocaMelodia();
void ClearScreen(int32_t color);
void EscribeFlash(void);
void UARTprintc(char c);
void UARTprint(const char * frase);
void UARTgets( char *BuffRx, int TMAX);

void TOCA_NOTA(unsigned int Nota);
void RELLENAR_COLOR(int32_t color,const Graphics_Rectangle *rectangulo);
void DIBUJAR_MARCO(int32_t color,const Graphics_Rectangle *flecha);

inline void inicia_ADC(char canales);
inline void InitCLK(void);
inline void InitBUZZ(void);
inline void InitUART(void);
inline void ConfigTimer1(unsigned int Period);
inline void InitScreen(const Graphics_Font *fuente);
inline void InitJoystick(void);
//------------------------------------------------------


//---------------------------------ESTRUCTURAS CREADAS---------------------------------------
typedef struct{
    char Nombre[3];
    unsigned int Puntuacion;
}Records;
//-------------------------------------------------------------------------------------------


//--------------------DECLARACIONES DE ESTRUCTURAS GLOBALES PARA LA PANTALLA---------------------------
Graphics_Rectangle ProgresoVacio   = {15,10,105,20};
Graphics_Rectangle ProgresoRelleno = {15,10,15,20};
//-----------------------------------------------------------------------------------------------------


// --------------------------------------- ENUM------------------------------------------
enum{
    Reposo,
    CompruebaSecuencia,
    MuestraRondaMaquina,
    RondaJugador,
    FinRonda,
    MuestraRondaJugador,
    Finjuego,
    NuevoRecord,
    MuestraRecords,
    Correcto,
    SecuenciaCorrecta
}Estados;

enum{
    Rojo,
    Verde,
    Azul,
    Amarillo,
    Vacio
}Rectangulos;

enum{
    PintarFlechaArriba,
    PintarFlechaDcha,
    PintarFlechaIzq,
    PintarFlechaAbajo,
    FlechaVacio
}Flechas;
//------------------------------------------------------------------------------------------


//----------------DECLARACION DE VARIABLES GLOBALES----------------------
volatile unsigned int tms = 0;
volatile unsigned int TiempoPintar = 0;
unsigned int T = TIEMPO_ILUMINADO;
volatile unsigned char TiempoCancion = 0;
volatile unsigned int Semilla;
char Secuencia[NUM_RONDAS];
unsigned int ejex, ejey;

Graphics_Context g_sContext;

Records Jugadores[5] = {{{'E','S','P'},64},{{'S','A','K'},4},{{'M','S','P'},2},{{'L','I','A'},1},{{'J','G','L'},1}};
//-----------------------------------------------------------------------

//--------- DECLARACION DE VARIABLES LOCALES DEL MAIN --------------------
char ColorPulsado = Vacio;
char Estado = Reposo;
char* pFlash= (char *) 0x1000; // APUNTO A LA DIRECCION 0x1000 DE LA FLASH
char Posicion = 0;
char RondaInt8[2];

unsigned int NuevaPuntuacion = 0;
char NuevaPuntuacionStr[20];
char Forma = 0;
unsigned int Ronda;
char Puesto = 0;//Para guardar el puesto que se debe de sustituir en los records
char MuyLento;
//-------------------------------------------------------------------------------


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

    unsigned int i,j;

    //--------------------CONFIGURACIONES INICIALES----------------------------------
    InitCLK();                    // SE CONFIGURA EL RELOJ A 16MHZ
    FCTL2 = FWKEY + FSSEL_2 + 34; // POR CONFIGURAR EL RELOJ A 16MHZ
    ConfigTimer1(29999);          //CONFIGURACIONES DEL TIMER 1
    InitBUZZ();                   //CONFIGURACIONES DEL BUZZER
    InitUART();                   //CONFIGURACIONES PARA EL PUERTO SERIE
    InitJoystick();               //CONFIGURACION DE LA ENTRADA DEL JOYSTICK
    InitScreen(&g_sFontCm16b);    //CONFIGURACIONES INICIALES DE PANTALLA
    //-------------------------------------------------------------------------------


    //----------------------------INICIALIZACION DE LA FLASH----------------------------------------
#if LIMPIAFLASH
    FCTL1 = FWKEY + ERASE;
    FCTL3 = FWKEY;
    pFlash[0] = 0;
    FCTL1 = FWKEY;               // Borra bit WRT
    FCTL3 = FWKEY + LOCK;        //activa LOCK
#endif

    //Solo cuando se escoja un micro que nunca se haya usado con este juego
    if((pFlash[25] != 17) || (pFlash[26] != 18)) EscribeFlash(); // Aqui los jugadores estan inicializados a Nombre = "AAA" y Puntuacion = 0


    //-------------------------INICIALIZACION DE LOS RECORDS A PARTIR DE LA FLASH-------------------
    for(i = 0, j = 0; i < 5; i++, j += 5)
    {
        Jugadores[i].Nombre[0]  = pFlash[j];
        Jugadores[i].Nombre[1]  = pFlash[j+1];
        Jugadores[i].Nombre[2]  = pFlash[j+2];

        Jugadores[i].Puntuacion = (pFlash[j+3] << 8) + pFlash[j+4];
    }
    //---------------------------------------------------------------------------------------------


    __bis_SR_register(GIE); // SE HABILITAN LAS INTERRUPCIONES GLOBALES

    while(1){
        LPM0;

        switch(Estado)
        {
        case Reposo:
            // SE DIBUJA LA PANTALLA DE INICIO
            Graphics_drawString(&g_sContext,"Modo Colores", 12, 20, 10, TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext,"Modo Formas",11, 20, 110, TRANSPARENT_TEXT);

            Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_RIGHT);
            Graphics_drawString(&g_sContext,"Records",6, 35, 10, TRANSPARENT_TEXT);
            Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

            ejex = lee_ch(0);
            ejey = lee_ch(3);

            if((ejey >= 700 && (ejex > 300) || Forma))
            {
                Estado = MuestraRondaMaquina;
                ClearScreen(GRAPHICS_COLOR_WHITE);

                GeneraSecuencia();// Se genera la secuencia de TAM_SECUENCIA numeros -> 0 = rojo, 1 = verde, 2 = azul, 3 = amarillo

                tms = 0;
                NuevaPuntuacion = 0;
                Ronda = 1;
                ProgresoRelleno.xMax = ProgresoVacio.xMin;
            }

            else if(ejex <= 300){
                Estado = MuestraRecords;
                ClearScreen(GRAPHICS_COLOR_WHITE);
            }

            else if(ejey <= 300) Forma = 1;

            break;
            //----------------------------------------------------------------------------------
        case MuestraRondaMaquina:
            // SE DIBUJA EL MENSAJE DE QUE COMIENZA LA RONDA N
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
            Graphics_drawString(&g_sContext,"Ronda: ", 20, 20, 64, TRANSPARENT_TEXT);
            sprintf(RondaInt8,"%d",Ronda);
            Graphics_drawString(&g_sContext,(int8_t*)RondaInt8, 20, 80, 64, TRANSPARENT_TEXT);

            if(tms > 70)
            {
                tms = 0;
                Estado = CompruebaSecuencia;
                ClearScreen(GRAPHICS_COLOR_BLACK);

                //SE DIBUJAN LOS COLORES APAGADOS
                if(!Forma) PintaRect(Vacio);
                else       PintaFlecha(Vacio);

            }
            break;
            //----------------------------------------------------------------------------------

            //----------------------------------------------------------------------------------
        case CompruebaSecuencia:
            if(tms < 2){
                if(Posicion != Ronda)// Si la posicion es distinta a la ronda por la que voy que pinte
                {
                    //--------------------- ESCALADO Y CALCULO DE LA BARRA DE PROGRESO----------------------------
                    if(((TAM_BARRA*100)/Ronda) & 0x09 >= 5) //Aqui hacemos algunas aproximaciones para redondear y rellenar el progreso
                        if(((((TAM_BARRA*10)/Ronda)+1) & 0x09) >= 5)
                            ProgresoRelleno.xMax += (TAM_BARRA/Ronda)+1;
                        else
                            ProgresoRelleno.xMax += (TAM_BARRA/Ronda);
                    else
                        ProgresoRelleno.xMax +=  (TAM_BARRA/Ronda);
                    //--------------------------------------------------------------------------------------------


                    if(!Forma) PintaRect(Secuencia[Posicion]);
                    else       PintaFlecha(Secuencia[Posicion]);

                    Posicion++;
                }
                else
                {
                    Estado = MuestraRondaJugador;
                    // SE LIMPIA LA PANTALLA PARA MOSTRAR EL MENSAJE DEL TURNO DEL JUGADOR
                    ClearScreen(GRAPHICS_COLOR_WHITE);

                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_drawString(&g_sContext,"Te Toca", 20, 20, 64, TRANSPARENT_TEXT);
                    tms = 0;

                    Posicion = 0;

                }
            }
            else if(tms >= (T+T/4))
            {
                tms = 0;
            }
            else if(tms >= T)
            {
                if(!Forma) PintaRect(Vacio);
                else       PintaFlecha(FlechaVacio);
            }
            break;
            //----------------------------------------------------------------------

        case MuestraRondaJugador:

            if(tms > T)
            {
                Estado = RondaJugador;

                if(!Posicion) ClearScreen(GRAPHICS_COLOR_BLACK);

                if(!Forma) PintaRect(Vacio);
                else       PintaFlecha(FlechaVacio);

                tms = 0;
            }

            break;



        case RondaJugador:
            ejex = lee_ch(0);
            ejey = lee_ch(3);

            if(ejey >= 700)
            {
                if(!Forma) PintaRect(Rojo);
                else       PintaFlecha(PintarFlechaArriba);
                tms = 0;
                ColorPulsado = Rojo;

            }
            if(ejey <= 300)
            {
                if(!Forma) PintaRect(Amarillo);
                else       PintaFlecha(PintarFlechaAbajo);
                tms = 0;
                ColorPulsado = Amarillo;

            }

            if(ejex >=700)
            {

                if(!Forma) PintaRect(Verde);
                else       PintaFlecha(PintarFlechaDcha);
                tms = 0;
                ColorPulsado = Verde;


            }
            if(ejex<=300)
            {
                if(!Forma) PintaRect(Azul);
                else       PintaFlecha(PintarFlechaIzq);
                tms = 0;
                ColorPulsado = Azul;

            }

            if(ColorPulsado != Vacio && ColorPulsado == Secuencia[Posicion]){ //SI SE HA SELECCIONADO UN COLOR Y HA SIDO CORRECTO
                if((Posicion+1) == Ronda) // SI SE HA LLEGADO AL FINAL DE LA SECUENCIA PARA LA RONDA ACTUAL
                {
                    Posicion = 0;
                    Ronda++;
                    NuevaPuntuacion++;
                    ProgresoRelleno.xMax = ProgresoVacio.xMin;
                    Estado = Correcto;
                    ColorPulsado = Vacio;
                    tms = 0;



                }
                else{ // SI NO SE HA LLEGADO AL FINAL DE LA SECUENCIA PARA LA RONDA ACTUAL
                    Posicion++;
                    NuevaPuntuacion++;
                    ColorPulsado = Vacio;
                    Estado = MuestraRondaJugador;
                    tms = 0;


                }
            }

            else if (ColorPulsado != Vacio || tms > 2*T)  // SI SE HA SELECCIONADO UN COLOR Y -NO- HA SIDO CORRECTO
            {
                Posicion = 0;
                Ronda = 1;
                Estado = Finjuego;
                ColorPulsado = Vacio;
                if(tms > 2*T) MuyLento = 1;
                else          MuyLento = 0;
                tms = 0;
            }

//PARA PODER SALTAR LAS RONDAS RAPIDO
#if PASARONDA
    Posicion = 0;
    Ronda++;
    NuevaPuntuacion++;
    ProgresoRelleno.xMax = ProgresoVacio.xMin;
   if(ColorPulsado == Vacio) Estado = Correcto;
    ColorPulsado = Vacio;
    tms = 0;
    FinCancion = 0;
#endif

            break;
            //-------------------------------------------------------------------------------------
        case Correcto:
            if(tms == T)
            {
                ClearScreen(GRAPHICS_COLOR_WHITE);
                TOCA_NOTA(0);
            }

            if(tms > T)
            {
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_drawString(&g_sContext,"Correcto!", 12, 20, 44, TRANSPARENT_TEXT);
            }

            if(tms > 50+T)
            {

                    TocaMelodia();

                tms = 0;
                Estado = FinRonda;

            }

            break;
            //-------------------------------------------------------------------------------------
        case FinRonda:
            if(tms >= 70)
            {
                tms = 0;
                TOCA_NOTA(0);

                Estado  = MuestraRondaMaquina;
                ClearScreen(GRAPHICS_COLOR_WHITE);
                if(Ronda>32){ Estado = SecuenciaCorrecta;}
            }
            break;

            //---------------------------------------------------------------------
        case SecuenciaCorrecta:
            if(tms > 100){
                Ronda=1;
                T/=2;

                // SE GENERA UNA NUEVA SECUENCIA ALEATORIA
                GeneraSecuencia();
                Estado = MuestraRondaMaquina;
                ClearScreen(GRAPHICS_COLOR_WHITE);
                tms = 0;
            }

            else if(tms > 50){
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_drawString(&g_sContext,"Secuencia", 9, 20, 44, TRANSPARENT_TEXT);
                Graphics_drawString(&g_sContext,"Completada!", 11, 20, 64, TRANSPARENT_TEXT);
            }

            break;
        case Finjuego:

            if(tms >= 60+T)
            {
                for(i = 0; i<5; i++)
                {
                    if(NuevaPuntuacion > Jugadores[i].Puntuacion) // LA PUNTUACION CONSEGUIDA HA SUPERADO ALGUNA DE LAS GUARDADAS
                    {
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                        Graphics_drawString(&g_sContext,"Introduce tus", 13, 15, 70, TRANSPARENT_TEXT);
                        Graphics_drawString(&g_sContext,"iniciales!", 10, 18, 83, TRANSPARENT_TEXT);

                        Graphics_drawString(&g_sContext,"Has batido un", 13, 15, 15, TRANSPARENT_TEXT);
                        Graphics_drawString(&g_sContext,"nuevo record!", 13, 15, 30, TRANSPARENT_TEXT);
                        Estado = NuevoRecord;
                        Puesto = i;
                        break;
                    }
                    Estado = Reposo;
                    Forma = 0;
                    tms = 0;
                }
            }

            else if(tms >= 50+T) ClearScreen(GRAPHICS_COLOR_WHITE);


            else if(tms >= 10+T)
            {
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                if(MuyLento) Graphics_drawString(&g_sContext,"Muy Lento!", 10, 20, 24, TRANSPARENT_TEXT);
                else         Graphics_drawString(&g_sContext,"Mal Elegido!", 11, 20, 24, TRANSPARENT_TEXT);

                Graphics_drawString(&g_sContext,"Has Perdido", 11, 20, 44, TRANSPARENT_TEXT);

                Graphics_drawString(&g_sContext,"Puntuacion: ",12,20,64,TRANSPARENT_TEXT);

                sprintf(NuevaPuntuacionStr,"%d",NuevaPuntuacion);
                Graphics_drawString(&g_sContext,(int8_t*)NuevaPuntuacionStr,20,20,84,TRANSPARENT_TEXT);

            }

            else if(tms >= T)
            {
                ClearScreen(GRAPHICS_COLOR_WHITE);
                TOCA_NOTA(0);
            }

            break;
            //-------------------------------------------------------------------------
        case NuevoRecord:
            UARTprint("\n\rIntroduce tus iniciales!: ");

            for(i = 4 - Puesto; i; i--){
                Jugadores[Puesto+i].Puntuacion = Jugadores[(Puesto+i)-1].Puntuacion;

                for(j = 0; j < 3; j++)
                {
                    Jugadores[Puesto+i].Nombre[j]  = Jugadores[(Puesto+i)-1].Nombre[j];
                }
            }

            Jugadores[Puesto].Puntuacion = NuevaPuntuacion;
            UARTgets(Jugadores[Puesto].Nombre,4);

            // UNA VEZ ACTUALIZADA LA ESTRUCTURA COMO QUEREMOS, LA GUARDAMOS EN FLASH
            EscribeFlash();

            ClearScreen(GRAPHICS_COLOR_WHITE);
            Estado = Reposo;
            Forma = 0;
            break;

            //-------------------------------------------------------------------------
        case MuestraRecords:
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
            Graphics_drawString(&g_sContext,"1)", 2, 10, 10, TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext,"2)", 2, 10, 30, TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext,"3)", 2, 10, 50, TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext,"4)", 2, 10, 70, TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext,"5)", 2, 10, 90, TRANSPARENT_TEXT);

            for(i = 0; i < 5; i++) {
                Graphics_drawString(&g_sContext,(int8_t*)Jugadores[i].Nombre, 3, 40, 10+20*i, TRANSPARENT_TEXT);
                sprintf(NuevaPuntuacionStr,"%d",Jugadores[i].Puntuacion);
                Graphics_drawString(&g_sContext,(int8_t*)NuevaPuntuacionStr, 20, 80, 10+20*i, TRANSPARENT_TEXT);
            }

            Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_RIGHT);
            Graphics_drawString(&g_sContext,"Volver", 6, 40, 110, TRANSPARENT_TEXT);
            Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

            ejex = lee_ch(0);
            ejey = lee_ch(3);
            if(ejex > 700){

                ClearScreen(GRAPHICS_COLOR_WHITE);
                Estado = Reposo;
                Forma = 0;
            }

            break;
            //-------------------------------------------------------------------------
        default:
            Estado = Reposo;
            Forma = 0;
            break;
        }

    }

}

void EscribeFlash(void){
    char* FLASH= (char *) 0x1000;
    unsigned int i,j;
    // UNA VEZ ACTUALIZADA LA ESTRUCTURA COMO QUEREMOS, LA GUARDAMOS EN FLASH
    FCTL1 = FWKEY + ERASE;
    FCTL3 = FWKEY;
    FLASH[0] = 0;

    // ACTIVO WRITE PARA REESCRIBIR LA FLASH
    FCTL1 = FWKEY + WRT;

    for(j = 0,i = 0; j < 25; j+=5, i++)
    {
        FLASH[j]   = Jugadores[i].Nombre[0];
        FLASH[j+1] = Jugadores[i].Nombre[1];
        FLASH[j+2] = Jugadores[i].Nombre[2];
        FLASH[j+3] = (Jugadores[i].Puntuacion >> 8) & 0xFF;
        FLASH[j+4] = Jugadores[i].Puntuacion & 0xFF;
    }

    FLASH[25] = 17;
    FLASH[26] = 18;

    // UNA VEZ ESCRITO, BORRO EL BIT WRITE Y ACTIVO EL BIT DE LOCK
    FCTL1 = FWKEY;               // Borra bit WRT
    FCTL3 = FWKEY + LOCK;        //activa LOCK
}

//-------------------------------------------------
void ClearScreen(int32_t color){
    Graphics_setBackgroundColor(&g_sContext, color);
    Graphics_clearDisplay(&g_sContext);
}
//-----------------------------------------------------
void TOCA_NOTA(unsigned int Nota){
    TA0CCR0 = Nota;
    TA0CCR1 = Nota>>1;
}
//-----------------------------------------------------
void RELLENAR_COLOR(int32_t color,const Graphics_Rectangle *rectangulo){
    Graphics_setForegroundColor(&g_sContext, color);
    Graphics_fillRectangle(&g_sContext,rectangulo);
}

//----------------------------------------------------------------------------------
void DIBUJAR_MARCO(int32_t color,const Graphics_Rectangle *marco){
    Graphics_setForegroundColor(&g_sContext, color);
    Graphics_drawRectangle(&g_sContext,marco);
}

//-----------------------------------------------------------------------------------
void PintaFlecha(char Valor){
    Graphics_Rectangle FlechaArriba    = {50,70,70,120};
    Graphics_Rectangle FlechaAbajo     = {50,40,70,90};
    Graphics_Rectangle FlechaIzq       = {50,70,100,90};
    Graphics_Rectangle FlechaDcha      = {20,70,70,90};

    Graphics_Rectangle clear  = {0,25,127,127};
    DIBUJAR_MARCO(GRAPHICS_COLOR_WHITE,&ProgresoVacio);

    RELLENAR_COLOR(GRAPHICS_COLOR_WHITE,&ProgresoRelleno);

    switch(Valor)
    {
    case PintarFlechaArriba:
           DIBUJAR_MARCO(GRAPHICS_COLOR_WHITE,&FlechaArriba);
           Graphics_drawLine(&g_sContext,40,70,80,70);
           Graphics_drawLine(&g_sContext,40,70,60,40);
           Graphics_drawLine(&g_sContext,60,40,80,70);
           TOCA_NOTA(DO);
           break;
           //---------------------------------------------------------------
       case PintarFlechaDcha:
           DIBUJAR_MARCO(GRAPHICS_COLOR_WHITE,&FlechaDcha);
           Graphics_drawLine(&g_sContext,70,60,70,100);
           Graphics_drawLine(&g_sContext,70,60,100,80);
           Graphics_drawLine(&g_sContext,70,100,100,80);
           TOCA_NOTA(MI);
           break;
           //---------------------------------------------------------------
       case PintarFlechaAbajo:
           DIBUJAR_MARCO(GRAPHICS_COLOR_WHITE,&FlechaAbajo);
           Graphics_drawLine(&g_sContext,40,90,80,90);
           Graphics_drawLine(&g_sContext,40,90,60,120);
           Graphics_drawLine(&g_sContext,60,120,80,90);
           TOCA_NOTA(SOL);
           break;
           //---------------------------------------------------------------
       case PintarFlechaIzq:
           DIBUJAR_MARCO(GRAPHICS_COLOR_WHITE,&FlechaIzq);
           Graphics_drawLine(&g_sContext,50,60,50,100);
           Graphics_drawLine(&g_sContext,50,60,20,80);
           Graphics_drawLine(&g_sContext,20,80,50,100);
           TOCA_NOTA(SI);
           break;
           //---------------------------------------------------------------
       case FlechaVacio:
           RELLENAR_COLOR(GRAPHICS_COLOR_BLACK,&clear);
           TOCA_NOTA(0);
           break;
    }

}

void PintaRect(char Valor){
    Graphics_Rectangle red    = {15,25,80,50};
    Graphics_Rectangle green  = {83,25,108,90};
    Graphics_Rectangle blue   = {15,53,40,118};
    Graphics_Rectangle yellow = {43,93,108,118};

    DIBUJAR_MARCO(GRAPHICS_COLOR_WHITE,&ProgresoVacio);

    RELLENAR_COLOR(GRAPHICS_COLOR_WHITE,&ProgresoRelleno);

    RELLENAR_COLOR(GRAPHICS_COLOR_DARK_RED,&red);
    RELLENAR_COLOR(GRAPHICS_COLOR_DARK_GREEN,&green);
    RELLENAR_COLOR(GRAPHICS_COLOR_DARK_BLUE,&blue);
    RELLENAR_COLOR(GRAPHICS_COLOR_YELLOW_GREEN,&yellow);

    switch(Valor)
    {
    case Rojo:
        RELLENAR_COLOR(GRAPHICS_COLOR_RED,&red);
        TOCA_NOTA(DO);
        break;
        //---------------------------------------------------------------
    case Verde:
        RELLENAR_COLOR(GRAPHICS_COLOR_LIME,&green);
        TOCA_NOTA(MI);
        break;
        //---------------------------------------------------------------
    case Amarillo:
        RELLENAR_COLOR(GRAPHICS_COLOR_YELLOW,&yellow);
        TOCA_NOTA(SOL);
        break;
        //---------------------------------------------------------------
    case Azul:
        RELLENAR_COLOR(GRAPHICS_COLOR_BLUE,&blue);
        TOCA_NOTA(SI);
        break;
        //---------------------------------------------------------------
    case Vacio:
        TOCA_NOTA(0);
        break;
    }
}
//--------------------------------------------------------------------------------------------------------------------------
void TocaMelodia(){

    TiempoCancion = 0;
    while(TiempoCancion < 10)
    {
            if(!TiempoCancion)
            {
                TOCA_NOTA(0);
            }

            else if(TiempoCancion < 5)
            {
                TOCA_NOTA(SI);
            }

            else
            {
                TOCA_NOTA(MI);
            }
        }

    TOCA_NOTA(0);
}

//-------------------------------- SOLO SE ACTIVA PARA LA COMPROBACION DE QUE SE GENERA MAS O MENOS LA MISMA CANTIDAD DE CADA NUMERO-----------


char Colores[4] = {0,0,0,0};
//--------------------------------------------------------------------------------------------------------

void GeneraSecuencia(void){
    // SE VA A UTILIZAR EL ALGORITMO DE GENERACION DE NUMEROS PSEUDOALEATORIOS "LINEAR FEEDBACK SHIFT REGISTER"
    unsigned int i;
    unsigned int LFSR = Semilla;

    for (i = 0; i < (NUM_RONDAS); i++)
    {
        Secuencia[i] = (LFSR>>3) & 0x03;
        unsigned int bit = LFSR & 0x01;
        LFSR >>= 1;
        if(bit) LFSR ^= FEEDBACK;

    }

#if CALCULA_DISTRIBUCION

    for(i = 0; i < 32; i++)
    {
        switch(Secuencia[i])
        {
        case Rojo: Colores[0]++; break;
        case Verde: Colores[1]++; break;
        case Azul:  Colores[2]++; break;
        case Amarillo: Colores[3]++; break;
        }
    }
#endif
}

void UARTprintc(char c){
	while (!(IFG2 & UCA0TXIFG)); 	//espera a Tx libre
			    	UCA0TXBUF = c;
}

void UARTprint(const char * frase){
	while(*frase)UARTprintc(*frase++);
}

void UARTgets( char *BuffRx, int TMAX){
	int indice=0;
	char caracter;
	do{
		while(!(IFG2&UCA0RXIFG));
		caracter=UCA0RXBUF;
		UARTprintc(caracter);
		if((caracter!=10)&&(caracter!=13)){BuffRx[indice]=caracter;
		indice++;}
	}while((caracter!=13)&&(indice<TMAX));

	if(indice==TMAX)indice--;
	BuffRx[indice]=0;
}


//---------------------------------------------------------------------------------------------------
inline void InitJoystick(void){
    P2DIR &= ~BIT5;
    P2OUT |= BIT5;
    inicia_ADC(BIT0+BIT3);
}
//----------------------------------------------------------------------------------------------------
inline void InitCLK(void){
    //CONFIGURACION DEL RELOJ A 16MHZ
    BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;

    if (CALBC1_16MHZ != 0xFF) {
        __delay_cycles(100000);
        DCOCTL = 0x00;
        BCSCTL1 = CALBC1_16MHZ;
        DCOCTL = CALDCO_16MHZ;
    }
    BCSCTL1 |= XT2OFF | DIVA_0;
    BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;
}
//-----------------------------------------------------------------------------
inline void InitBUZZ(void){

    P2DIR|=BIT6;            
    P2SEL|= BIT6;          
    P2SEL2&=~(BIT6);

    TA0CTL=TASSEL_2| MC_1;  
    TA0CCTL1=OUTMOD_7;      

}
//------------------------------------------------------------------------------
inline void InitUART(void){
    P1DIR = BIT0 + BIT2;
    P1REN = BIT3;

    P1SEL2|= BIT1 | BIT2;  
    P1SEL|= BIT1 | BIT2;

    UCA0CTL1 |= UCSWRST;          
    UCA0CTL1 = UCSSEL_2 | UCSWRST; 
    UCA0BR0 =139;
    UCA0CTL1 &= ~UCSWRST;          
    IFG2 &= ~(UCA0RXIFG);          
    IE2 &=~ UCA0RXIE;              
}
//-------------------------------------------------------------------------------
inline void ConfigTimer1(unsigned int Period){
    TA1CTL=TASSEL_2|ID_3 | MC_3;        
    TA1CCR0=Period;
    TA1CCTL0=CCIE;
}
//----------------------------------------------------------------------------------------------------
inline void InitScreen(const Graphics_Font *fuente){
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
    Graphics_setFont(&g_sContext, fuente);
    ClearScreen(GRAPHICS_COLOR_WHITE);
}
//-----------------------------------------------------------------------------------------------------
inline void inicia_ADC(char canales){
    ADC10CTL0 &= ~ENC;      
    ADC10CTL0 = ADC10ON | ADC10SHT_3 | SREF_0|ADC10IE; 
    ADC10CTL1 = CONSEQ_0 | ADC10SSEL_0 | ADC10DIV_0 | SHS_0 | INCH_0;
    ADC10AE0 = canales; 
    ADC10CTL0 |= ENC; 
}
//-------------------------------------------------------------------------------
int lee_ch(char canal){
    ADC10CTL0 &= ~ENC;                 
    ADC10CTL1&=(0x0fff);               
    ADC10CTL1|=canal<<12;              
    ADC10CTL0|= ENC;                   
    ADC10CTL0|=ADC10SC;                
    LPM0;                             
    return(ADC10MEM);                   
}


#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_200ms(void)
{
    Semilla++;
    tms++;
    TiempoPintar++;
    TiempoCancion++;

    LPM0_EXIT;
}


#pragma vector=ADC10_VECTOR
__interrupt void ConvertidorAD(void)
{
    LPM0_EXIT;  //Despierta al micro al final de la conversion
}

