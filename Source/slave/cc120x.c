/*****************************************************************************/
// @file 	cc120x.c
//
// @brief 	Implementation file for basic and neccessary functions
//          to communicate with CC120X over SPI
//
//  Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/****************************************************************************/

/******************************************************************************
 * INCLUDES
 */
#include "cc120x.h"
#include "common.h"
#include "device.h"

/******************************************************************************
 * CC120X SPI DEFINE & MACRO
 */
#define CC120X_SPI_MOSI_P3_PIN_BM   BM(1)
#define CC120X_SPI_MISO_P3_PIN_BM   BM(2)
#define CC120X_SPI_SCLK_P3_PIN_BM   BM(3)
#define CC120X_SPI_NSS_P3_PIN_BM    BM(0)

#define CC120X_SPI_NSS_LOW()                                                                                           \
    do {                                                                                                               \
        P3SEL &= ~CC120X_SPI_NSS_P3_PIN_BM;                                                                            \
        P3DIR |= CC120X_SPI_NSS_P3_PIN_BM;                                                                             \
        P3OUT &= ~CC120X_SPI_NSS_P3_PIN_BM;                                                                            \
    } while (0);

#define CC120X_SPI_NSS_HIGH()                                                                                          \
    do {                                                                                                               \
        P3SEL &= ~CC120X_SPI_NSS_P3_PIN_BM;                                                                            \
        P3DIR |= CC120X_SPI_NSS_P3_PIN_BM;                                                                             \
        P3OUT |= CC120X_SPI_NSS_P3_PIN_BM;                                                                             \
    } while (0);

typedef enum {
    CC120X_SPI_MODE_0 = 0,
    CC120X_SPI_MODE_1 = 1,
    CC120X_SPI_MODE_2 = 2,
    CC120X_SPI_MODE_3 = 3,
} CC120X_SPI_MODE_t;

/******************************************************************************
 * CC120X SPI FUNCTIONS
 */
void cc120xSpiEnable(uint8 prescaler, CC120X_SPI_MODE_t mode)
{
    /* Keep peripheral in reset state*/
    UCB0CTL1 |= UCSWRST;

    /* Configuration
     * -  8-bit
     * -  Master Mode
     * -  3-pin
     * -  synchronous mode
     * -  MSB first
     * -  Clock phase select = captured on first edge
     * -  Inactive state is low
     * -  SMCLK as clock source
     * -  Spi clk is adjusted corresponding to systemClock as the highest rate
     *    supported by the supported radios: this could be optimized and done
     *    after chip detect.
     */
    UCB0CTL0 = UCMST + UCSYNC + UCMODE_0 + UCMSB; // CC1200 : MODE #0, KSHELL : MODE #2
    switch (mode) {
    case CC120X_SPI_MODE_0:
        UCB0CTL0 |= (UCCKPH);
        break;
    case CC120X_SPI_MODE_1: /* Do not set */
        break;
    case CC120X_SPI_MODE_2:
        UCB0CTL0 |= (UCCKPH + UCCKPL);
        break;
    case CC120X_SPI_MODE_3:
        UCB0CTL0 |= (UCCKPL);
        break;
    default:
        while (1);
    }

    // Set source clock to SMCLK
    UCB0CTL1 |= UCSSEL_2;

    /* Configuring UCB0BR0
     * Set up spi clk to comply with the maximum spi clk speed for the radios
     * according to userguides. Takes the slowest spi clk into account.
     * SMCLK divide.
     */
    UCB0BR0 = prescaler; // no divide
    UCB0BR1 = 0;

    // MISO/MOSI/SCLK GPIO controlled by peripheral
    P3SEL |= CC120X_SPI_MOSI_P3_PIN_BM + CC120X_SPI_MISO_P3_PIN_BM + CC120X_SPI_SCLK_P3_PIN_BM;
    // NSS Output High for deactive slave
    CC120X_SPI_NSS_HIGH();

    /* Release for operation */
    UCB0CTL1 &= ~UCSWRST;

    return;
}

void cc120xSpiDisable()
{
    P3SEL &= ~(CC120X_SPI_MOSI_P3_PIN_BM + CC120X_SPI_MISO_P3_PIN_BM + CC120X_SPI_SCLK_P3_PIN_BM);
    P3DIR |= (CC120X_SPI_MOSI_P3_PIN_BM + CC120X_SPI_MISO_P3_PIN_BM + CC120X_SPI_SCLK_P3_PIN_BM);
    P3OUT &= ~(CC120X_SPI_MOSI_P3_PIN_BM + CC120X_SPI_MISO_P3_PIN_BM + CC120X_SPI_SCLK_P3_PIN_BM);

    CC120X_SPI_NSS_LOW();

    UCB0CTL1 |= UCSWRST;
}

void cc120xSpiBegin()
{
    CC120X_SPI_NSS_LOW();
}

void cc120xSpiEnd()
{
    CC120X_SPI_NSS_HIGH();
}

void cc120xSpiSend(uint8 data)
{
    while (!(UCB0IFG & UCTXIFG)); // check Tx is ready
    UCB0TXBUF = (data);
}

void cc120xSpiWaitBusy()
{
    while (UCB0STAT & UCBUSY);
}

uint8 cc120xSpiRecv()
{
    while (!(UCB0IFG & UCRXIFG)); // check Rx is ready
    UCB0IFG &= ~UCRXIFG;

    return (UCB0RXBUF);
}

/******************************************************************************
 * CC120X DEFINE & MACRO
 */
#define RADIO_BURST_ACCESS              0x40
#define RADIO_SINGLE_ACCESS             0x00
#define RADIO_READ_ACCESS               0x80
#define RADIO_WRITE_ACCESS              0x00

/* Bit fields in the chip status byte */
#define STATUS_CHIP_RDYn_BM             0x80
#define STATUS_STATE_BM                 0x70
#define STATUS_FIFO_BYTES_AVAILABLE_BM  0x0F

/******************************************************************************
 * CC120X FUNCTIONS
 */

/*******************************************************************************
 * @fn          cc120xRwBurstSingle
 *
 * @brief       When the address byte is sent to the SPI slave, the next byte
 *              communicated is the data to be written or read. The address
 *              byte that holds information about read/write -and single/
 *              burst-access is provided to this function.
 *
 *              Depending on these two bits this function will write len bytes to
 *              the radio in burst mode or read len bytes from the radio in burst
 *              mode if the burst bit is set. If the burst bit is not set, only
 *              one data byte is communicated.
 *
 *              NOTE: This function is used in the following way:
 *
 *              cc120xSpiBegin();
 *              ...[Depending on type of register access]
 *              cc120xRwBurstSingle(uint8 addr,uint8 *pData,uint16 len);
 *              cc120xSpiEnd();
 *
 * input parameters
 *
 * @param       none
 *
 * output parameters
 *
 * @return      void
 */
static void cc120xRwBurstSingle(uint8 addr, uint8* pData, uint16 len)
{
    uint16 i;
    /* Communicate len number of bytes: if RX - the procedure sends 0x00 to push bytes from slave*/
    if (addr & RADIO_READ_ACCESS) {
        if (addr & RADIO_BURST_ACCESS) {
            for (i = 0; i < len; i++) {
                cc120xSpiSend(0); /* Possible to combining read and write as one access type */
                cc120xSpiWaitBusy();
                *pData = cc120xSpiRecv(); /* Store pData from last pData RX */
                pData++;
            }
        } else {
            cc120xSpiSend(0);
            cc120xSpiWaitBusy();
            *pData = cc120xSpiRecv();
        }
    } else {
        if (addr & RADIO_BURST_ACCESS) {
            /* Communicate len number of bytes: if TX - the procedure doesn't overwrite pData */
            for (i = 0; i < len; i++) {
                cc120xSpiSend(*pData);
                cc120xSpiWaitBusy();
                pData++;
            }
        } else {
            cc120xSpiSend(*pData);
            cc120xSpiWaitBusy();
        }
    }
    return;
}

/*******************************************************************************
 * @fn          cc120xRead8bitReg
 *
 * @brief       This function performs a read or write from/to a 8bit register
 *              address space. The function handles burst and single read/write
 *              as specfied in addrByte. Function assumes that chip is ready.
 *
 * input parameters
 *
 * @param       accessType - Specifies if this is a read or write and if it's
 *                           a single or burst access. Bitmask made up of
 *                           RADIO_BURST_ACCESS/RADIO_SINGLE_ACCESS/
 *                           RADIO_WRITE_ACCESS/RADIO_READ_ACCESS.
 * @param       addrByte - address byte of register.
 * @param       pData    - data array
 * @param       len      - Length of array to be read(TX)/written(RX)
 *
 * output parameters
 *
 * @return      chip status
 */
static rfStatus_t cc120xRead8bitReg(uint8 accessType, uint8 addrByte, uint8* pData, uint16 len)
{
    uint8 readValue;

    cc120xSpiBegin();
    while (P3IN & CC120X_SPI_MISO_P3_PIN_BM);
    /* send register address byte */
    cc120xSpiSend(accessType | addrByte);
    cc120xSpiWaitBusy();
    /* Storing chip status */
    readValue = cc120xSpiRecv();
    cc120xRwBurstSingle(accessType | addrByte, pData, len);
    cc120xSpiEnd();
    /* return the status byte value */
    return (readValue);
}

/******************************************************************************
 * @fn          cc120xRead16bitReg
 *
 * @brief       This function performs a read or write in the extended adress
 *              space of CC112X.
 *
 * input parameters
 *
 * @param       accessType - Specifies if this is a read or write and if it's
 *                           a single or burst access. Bitmask made up of
 *                           RADIO_BURST_ACCESS/RADIO_SINGLE_ACCESS/
 *                           RADIO_WRITE_ACCESS/RADIO_READ_ACCESS.
 * @param       extAddr - Extended register space address = 0x2F.
 * @param       regAddr - Register address in the extended address space.
 * @param       *pData  - Pointer to data array for communication
 * @param       len     - Length of bytes to be read/written from/to radio
 *
 * output parameters
 *
 * @return      rfStatus_t
 */
static rfStatus_t cc120xRead16bitReg(uint8 accessType, uint8 extAddr, uint8 regAddr, uint8* pData, uint8 len)
{
    uint8 readValue;

    cc120xSpiBegin();
    while (P3IN & CC120X_SPI_MISO_P3_PIN_BM)
        ;
    /* send extended address byte with access type bits set */
    cc120xSpiSend(accessType | extAddr);
    cc120xSpiWaitBusy();
    /* Storing chip status */
    readValue = cc120xSpiRecv();
    cc120xSpiSend(regAddr);
    cc120xSpiWaitBusy();
    /* Communicate len number of bytes */
    cc120xRwBurstSingle(accessType | extAddr, pData, len);
    cc120xSpiEnd();
    /* return the status byte value */
    return (readValue);
}

/******************************************************************************
 * @fn          CC120X_init
 *
 * @brief       Function to initialize TRX SPI. CC1101/CC112x is currently
 *              supported. The supported prescalerValue must be set so that
 *              SMCLK/prescalerValue does not violate radio SPI constraints.
 *
 * input parameters
 *
 * @param       prescalerValue - SMCLK/prescalerValue gives SCLK frequency
 *
 * output parameters
 *
 * @return      void
 */
void CC120X_init()
{
    cc120xSpiEnable(2, CC120X_SPI_MODE_0);
}

/*******************************************************************************
 * @fn          CC120X_strobeCmd
 *
 * @brief       Send command strobe to the radio. Returns status byte read
 *              during transfer of command strobe. Validation of provided
 *              is not done. Function assumes chip is ready.
 *
 * input parameters
 *
 * @param       cmd - command strobe
 *
 * output parameters
 *
 * @return      status byte
 */
rfStatus_t CC120X_strobeCmd(uint8 cmd)
{
    uint8 rc;
    cc120xSpiBegin();
    while (P3IN & CC120X_SPI_MISO_P3_PIN_BM);
    cc120xSpiSend(cmd);
    cc120xSpiWaitBusy();
    rc = cc120xSpiRecv();
    cc120xSpiEnd();
    return (rc);
}

/******************************************************************************
 * @fn          CC120X_readReg
 *
 * @brief       Read value(s) from config/status/extended radio register(s).
 *              If len  = 1: Reads a single register
 *              if len != 1: Reads len register values in burst mode
 *
 * input parameters
 *
 * @param       addr   - address of first register to read
 * @param       *pData - pointer to data array where read bytes are saved
 * @param       len   - number of bytes to read
 *
 * output parameters
 *
 * @return      rfStatus_t
 */
rfStatus_t CC120X_readReg(uint16 addr, uint8* pData, uint8 len)
{
    uint8 tempExt = (uint8)(addr >> 8);
    uint8 tempAddr = (uint8)(addr & 0x00FF);
    uint8 rc;

    /* Checking if this is a FIFO access -> returns chip not ready  */
    if ((CC120X_SINGLE_TXFIFO <= tempAddr) && (tempExt == 0))
        return STATUS_CHIP_RDYn_BM;

    /* Decide what register space is accessed */
    if (!tempExt) {
        rc = cc120xRead8bitReg((RADIO_BURST_ACCESS | RADIO_READ_ACCESS), tempAddr, pData, len);
    } else if (tempExt == 0x2F) {
        rc = cc120xRead16bitReg((RADIO_BURST_ACCESS | RADIO_READ_ACCESS), tempExt, tempAddr, pData, len);
    }
    return (rc);
}

/******************************************************************************
 * @fn          CC120X_writeReg
 *
 * @brief       Write value(s) to config/status/extended radio register(s).
 *              If len  = 1: Writes a single register
 *              if len  > 1: Writes len register values in burst mode
 *
 * input parameters
 *
 * @param       addr   - address of first register to write
 * @param       *pData - pointer to data array that holds bytes to be written
 * @param       len    - number of bytes to write
 *
 * output parameters
 *
 * @return      rfStatus_t
 */
rfStatus_t CC120X_writeReg(uint16 addr, uint8* pData, uint8 len)
{
    uint8 tempExt = (uint8)(addr >> 8);
    uint8 tempAddr = (uint8)(addr & 0x00FF);
    uint8 rc;

    /* Checking if this is a FIFO access - returns chip not ready */
    if ((CC120X_SINGLE_TXFIFO <= tempAddr) && (tempExt == 0))
        return STATUS_CHIP_RDYn_BM;

    /* Decide what register space is accessed */
    if (!tempExt) {
        rc = cc120xRead8bitReg((RADIO_BURST_ACCESS | RADIO_WRITE_ACCESS), tempAddr, pData, len);
    } else if (tempExt == 0x2F) {
        rc = cc120xRead16bitReg((RADIO_BURST_ACCESS | RADIO_WRITE_ACCESS), tempExt, tempAddr, pData, len);
    }
    return (rc);
}

/*******************************************************************************
 * @fn          CC120X_writeTxFifo
 *
 * @brief       Write pData to radio transmit FIFO.
 *
 * input parameters
 *
 * @param       *pData - pointer to data array that is written to TX FIFO
 * @param       len    - Length of data array to be written
 *
 * output parameters
 *
 * @return      rfStatus_t
 */
rfStatus_t CC120X_writeTxFifo(uint8* pData, uint8 len)
{
    uint8 rc;
    rc = cc120xRead8bitReg(0x00, CC120X_BURST_TXFIFO, pData, len);
    return (rc);
}

/*******************************************************************************
 * @fn          CC120X_readRxFifo
 *
 * @brief       Reads RX FIFO values to pData array
 *
 * input parameters
 *
 * @param       *pData - pointer to data array where RX FIFO bytes are saved
 * @param       len    - number of bytes to read from the RX FIFO
 *
 * output parameters
 *
 * @return      rfStatus_t
 */
rfStatus_t CC120X_readRxFifo(uint8* pData, uint8 len)
{
    uint8 rc;
    rc = cc120xRead8bitReg(0x00, CC120X_BURST_RXFIFO, pData, len);
    return (rc);
}

/******************************************************************************
 * @fn      CC120X_getTxStatus(void)
 *
 * @brief   This function transmits a No Operation Strobe (SNOP) to get the
 *          status of the radio and the number of free bytes in the TX FIFO.
 *
 *          Status byte:
 *
 *          ---------------------------------------------------------------------------
 *          |          |            |                                                 |
 *          | CHIP_RDY | STATE[2:0] | FIFO_BYTES_AVAILABLE (free bytes in the TX FIFO |
 *          |          |            |                                                 |
 *          ---------------------------------------------------------------------------
 *
 *
 * input parameters
 *
 * @param   none
 *
 * output parameters
 *
 * @return  rfStatus_t
 *
 */
rfStatus_t CC120X_getTxStatus(void)
{
    return (CC120X_strobeCmd(CC120X_SNOP));
}

/******************************************************************************
 *
 *  @fn       CC120X_getRxStatus(void)
 *
 *  @brief
 *            This function transmits a No Operation Strobe (SNOP) with the
 *            read bit set to get the status of the radio and the number of
 *            available bytes in the RXFIFO.
 *
 *            Status byte:
 *
 *            --------------------------------------------------------------------------------
 *            |          |            |                                                      |
 *            | CHIP_RDY | STATE[2:0] | FIFO_BYTES_AVAILABLE (available bytes in the RX FIFO |
 *            |          |            |                                                      |
 *            --------------------------------------------------------------------------------
 *
 *
 * input parameters
 *
 * @param     none
 *
 * output parameters
 *
 * @return    rfStatus_t
 *
 */
rfStatus_t CC120X_getRxStatus(void)
{
    return (CC120X_strobeCmd(CC120X_SNOP | RADIO_READ_ACCESS));
}
