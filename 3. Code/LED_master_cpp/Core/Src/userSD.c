
#include "UserSD.h"

#define TRUE  1
#define FALSE 0
#define bool BYTE

///* defines for the CS PIN */
//#define SD_CS_GPIO_Port SD_CS_GPIO_Port
//#define SD_CS_Pin SD_CS_Pin

volatile uint8_t FatFsCnt=0, Timer1=0, Timer2=0;
static volatile DSTATUS Stat = STA_NOINIT;              /* Disc Status Flag*/
static uint8_t CardType;                                /* SD type 0:MMC, 1:SDC, 2:Block addressing */
static uint8_t PowerFlag = 0;                           /* Power condition Flag */

void SD_1msTick(void)
{
	FatFsCnt++;
	if(FatFsCnt >= 10)
	{
		FatFsCnt = 0;
		if(Timer1 > 0) Timer1--;
		if(Timer2 > 0) Timer2--;
	}
}

/* SPI Chip Select */
static void SELECT(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

/* SPI Chip Deselect */
static void DESELECT(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

/* SPI Transmit*/
static void SPI_TxByte(BYTE data)
{
  while ((HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY));
  HAL_SPI_Transmit(&hspi1, &data, 1, SPI_TIMEOUT);
}

/* SPI Data send / receive return type function */
static uint8_t SPI_RxByte(void)
{
  uint8_t dummy, data;
  dummy = 0xFF;
  data = 0;

  while ((HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY));
 HAL_SPI_TransmitReceive(&hspi1, &dummy, &data, 1, SPI_TIMEOUT);

  return data;
}

/* SD CARD Ready wait */
static uint8_t SD_ReadyWait(void)
{
  uint8_t res;

  /* 500ms Counter preparation*/
  Timer2 = 50;

  SPI_RxByte();

  do
  {
    /* 0xFF SPI communication until a value is received */
    res = SPI_RxByte();
  } while ((res != 0xFF) && Timer2);

  return res;
}

/*Power on*/
static void SD_PowerOn(void)
{
  uint8_t cmd_arg[6];
  uint32_t Count = 0x1FFF;


  DESELECT();

  for(int i = 0; i < 10; i++)
  {
    SPI_TxByte(0xFF);
  }

  /* SPI Chips Select */
  SELECT();

  /*  GO_IDLE_STATE State transitions*/
  cmd_arg[0] = (CMD0 | 0x40);
  cmd_arg[1] = 0;
  cmd_arg[2] = 0;
  cmd_arg[3] = 0;
  cmd_arg[4] = 0;
  cmd_arg[5] = 0x95;

  /* Command transmission*/
  for (int i = 0; i < 6; i++)
  {
    SPI_TxByte(cmd_arg[i]);
  }

  /* Answer waiting*/
  while ((SPI_RxByte() != 0x01) && Count)
  {
    Count--;
  }

  DESELECT();
  SPI_TxByte(0XFF);

  PowerFlag = 1;
}

/* 전원 끄기 */
static void SD_PowerOff(void)
{
  PowerFlag = 0;
}

/* 전원 상태 확인 */
static uint8_t SD_CheckPower(void)
{
  /*  0=off, 1=on */
  return PowerFlag;
}

/* 데이터 패킷 수신 */
static bool SD_RxDataBlock(BYTE *buff, UINT btr)
{
  uint8_t token = 0xFF;

  /* 100ms 타이머 */
  Timer1 = 10;

  /* 응답 대기 */
  do
  {
    token = SPI_RxByte();
  } while((token == 0xFF) && Timer1);

  /* 0xFE 이외 Token 수신 시 에러 처리 */
  if(token != 0xFE)
    return FALSE;

	//for read with DMA:
	uint8_t dummy=0xFF;
	while ((HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY));
	HAL_SPI_TransmitReceive_DMA(&hspi1, &dummy, buff, btr);

//  //for read without DMA:
//  do
//  {
//	*buff = SPI_RxByte(); buff++;
//	*buff = SPI_RxByte(); buff++;
//  } while(btr -= 2);

  SPI_RxByte(); /* CRC 무시 */
  SPI_RxByte();

  return TRUE;
}

/* 데이터 전송 패킷 */
#if _READONLY == 0
static bool SD_TxDataBlock(const BYTE *buff, BYTE token)
{
  uint8_t respon = 0, wc = 0;
  uint8_t i = 0;

  /* SD카드 준비 대기 */
  if (SD_ReadyWait() != 0xFF)
    return FALSE;

  /* 토큰 전송 */
  SPI_TxByte(token);

  /* 데이터 토큰인 경우 */
  if (token != 0xFD)
  {
    wc = 0;

    /* 512 바이트 데이터 전송 */
    do
    {
      SPI_TxByte(*buff++);
      SPI_TxByte(*buff++);
    } while (--wc);

    SPI_RxByte();       /* CRC 무시 */
    SPI_RxByte();

    /* 데이트 응답 수신 */
    while (i <= 64)
    {
      respon = SPI_RxByte();

      /* 에러 응답 처리 */
      if ((respon & 0x1F) == 0x05)
        break;

      i++;
    }

    /* SPI 수신 버퍼 Clear */
    while (SPI_RxByte() == 0);
  }

  if ((respon & 0x1F) == 0x05)
    return TRUE;
  else
    return FALSE;
}
#endif /* _READONLY */

/* CMD 패킷 전송 */
static BYTE SD_SendCmd(BYTE cmd, DWORD arg)
{
  uint8_t crc, res;

  /* SD카드 대기 */
  if (SD_ReadyWait() != 0xFF)
    return 0xFF;

  /* 명령 패킷 전송 */
  SPI_TxByte(cmd); 			/* Command */
  SPI_TxByte((BYTE) (arg >> 24)); 	/* Argument[31..24] */
  SPI_TxByte((BYTE) (arg >> 16)); 	/* Argument[23..16] */
  SPI_TxByte((BYTE) (arg >> 8)); 	/* Argument[15..8] */
  SPI_TxByte((BYTE) arg); 		/* Argument[7..0] */

  /* 명령별 CRC 준비 */
  crc = 0;
  if (cmd == CMD0)
    crc = 0x95; /* CRC for CMD0(0) */

  if (cmd == CMD8)
    crc = 0x87; /* CRC for CMD8(0x1AA) */

  /* CRC 전송 */
  SPI_TxByte(crc);

  /* CMD12 Stop Reading 명령인 경우에는 응답 바이트 하나를 버린다 */
  if (cmd == CMD12)
    SPI_RxByte();

  /* 10회 내에 정상 데이터를 수신한다. */
  uint8_t n = 10;
  do
  {
    res = SPI_RxByte();
  } while ((res & 0x80) && --n);

  return res;
}

/*-----------------------------------------------------------------------
  fatfs에서 사용되는 Global 함수들
  user_diskio.c 파일에서 사용된다.
-----------------------------------------------------------------------*/

/* SD카드 초기화 */
DSTATUS SD_disk_initialize(BYTE drv)
{
  uint8_t n, type, ocr[4];

  /* 한종류의 드라이브만 지원 */
  if(drv)
    return STA_NOINIT;

  /* SD카드 미삽입 */
  if(Stat & STA_NODISK)
    return Stat;

  /* SD카드 Power On */
  SD_PowerOn();

  /* SPI 통신을 위해 Chip Select */
  SELECT();

  /* SD카드 타입변수 초기화 */
  type = 0;

  /* Idle 상태 진입 */
  if (SD_SendCmd(CMD0, 0) == 1)
  {
    /* 타이머 1초 설정 */
    Timer1 = 100;

    /* SD 인터페이스 동작 조건 확인 */
    if (SD_SendCmd(CMD8, 0x1AA) == 1)
    {
      /* SDC Ver2+ */
      for (n = 0; n < 4; n++)
      {
        ocr[n] = SPI_RxByte();
      }

      if (ocr[2] == 0x01 && ocr[3] == 0xAA)
      {
        /* 2.7-3.6V 전압범위 동작 */
        do {
          if (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 1UL << 30) == 0)
            break; /* ACMD41 with HCS bit */
        } while (Timer1);

        if (Timer1 && SD_SendCmd(CMD58, 0) == 0)
        {
          /* Check CCS bit */
          for (n = 0; n < 4; n++)
          {
            ocr[n] = SPI_RxByte();
          }

          type = (ocr[0] & 0x40) ? 6 : 2;
        }
      }
    }
    else
    {
      /* SDC Ver1 or MMC */
      type = (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) <= 1) ? 2 : 1; /* SDC : MMC */

      do {
        if (type == 2)
        {
          if (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) == 0)
            break; /* ACMD41 */
        }
        else
        {
          if (SD_SendCmd(CMD1, 0) == 0)
            break; /* CMD1 */
        }
      } while (Timer1);

      if (!Timer1 || SD_SendCmd(CMD16, 512) != 0)
      {
        /* 블럭 길이 선택 */
        type = 0;
      }
    }
  }

  CardType = type;

  DESELECT();

  SPI_RxByte(); /* Idle 상태 전환 (Release DO) */

  if (type)
  {
    /* Clear STA_NOINIT */
    Stat &= ~STA_NOINIT;
  }
  else
  {
    /* Initialization failed */
    SD_PowerOff();
  }

  return Stat;
}

/* 디스크 상태 확인 */
DSTATUS SD_disk_status(BYTE drv)
{
  if (drv)
    return STA_NOINIT;

  return Stat;
}

/* 섹터 읽기 */
DRESULT SD_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count)
{
  if (pdrv || !count)
    return RES_PARERR;

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;

  if (!(CardType & 4))
    sector *= 512;      /* 지정 sector를 Byte addressing 단위로 변경 */

  SELECT();

  if (count == 1)
  {
    /* 싱글 블록 읽기 */
    if ((SD_SendCmd(CMD17, sector) == 0) && SD_RxDataBlock(buff, 512))
      count = 0;
  }
  else
  {
    /* 다중 블록 읽기 */
    if (SD_SendCmd(CMD18, sector) == 0)
    {
      do {
        if (!SD_RxDataBlock(buff, 512))
          break;

        buff += 512;
      } while (--count);

      /* STOP_TRANSMISSION, 모든 블럭을 다 읽은 후, 전송 중지 요청 */
      SD_SendCmd(CMD12, 0);
    }
  }

  DESELECT();
  SPI_RxByte(); /* Idle 상태(Release DO) */

  return count ? RES_ERROR : RES_OK;
}

/* 섹터 쓰기 */
#if _READONLY == 0
DRESULT SD_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
  if (pdrv || !count)
    return RES_PARERR;

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;

  if (Stat & STA_PROTECT)
    return RES_WRPRT;

  if (!(CardType & 4))
    sector *= 512; /* 지정 sector를 Byte addressing 단위로 변경 */

  SELECT();

  if (count == 1)
  {
    /* 싱글 블록 쓰기 */
    if ((SD_SendCmd(CMD24, sector) == 0) && SD_TxDataBlock(buff, 0xFE))
      count = 0;
  }
  else
  {
    /* 다중 블록 쓰기 */
    if (CardType & 2)
    {
      SD_SendCmd(CMD55, 0);
      SD_SendCmd(CMD23, count); /* ACMD23 */
    }

    if (SD_SendCmd(CMD25, sector) == 0)
    {
      do {
        if(!SD_TxDataBlock(buff, 0xFC))
          break;

        buff += 512;
      } while (--count);

      if(!SD_TxDataBlock(0, 0xFD))
      {
        count = 1;
      }
    }
  }

  DESELECT();
  SPI_RxByte();

  return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY */

/* 기타 함수 */
DRESULT SD_disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
  DRESULT res;
  BYTE n, csd[16], *ptr = buff;
  WORD csize;

  if (drv)
    return RES_PARERR;

  res = RES_ERROR;

  if (ctrl == CTRL_POWER)
  {
    switch (*ptr)
    {
    case 0:
      if (SD_CheckPower())
        SD_PowerOff();          /* Power Off */
      res = RES_OK;
      break;
    case 1:
      SD_PowerOn();             /* Power On */
      res = RES_OK;
      break;
    case 2:
      *(ptr + 1) = (BYTE) SD_CheckPower();
      res = RES_OK;             /* Power Check */
      break;
    default:
      res = RES_PARERR;
    }
  }
  else
  {
    if (Stat & STA_NOINIT)
      return RES_NOTRDY;

    SELECT();

    switch (ctrl)
    {
    case GET_SECTOR_COUNT:
      /* SD카드 내 Sector의 개수 (DWORD) */
      if ((SD_SendCmd(CMD9, 0) == 0) && SD_RxDataBlock(csd, 16))
      {
        if ((csd[0] >> 6) == 1)
        {
          /* SDC ver 2.00 */
          csize = csd[9] + ((WORD) csd[8] << 8) + 1;
          *(DWORD*) buff = (DWORD) csize << 10;
        }
        else
        {
          /* MMC or SDC ver 1.XX */
          n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
          csize = (csd[8] >> 6) + ((WORD) csd[7] << 2) + ((WORD) (csd[6] & 3) << 10) + 1;
          *(DWORD*) buff = (DWORD) csize << (n - 9);
        }

        res = RES_OK;
      }
      break;

    case GET_SECTOR_SIZE:
      /* 섹터의 단위 크기 (WORD) */
      *(WORD*) buff = 512;
      res = RES_OK;
      break;

    case CTRL_SYNC:
      /* 쓰기 동기화 */
      if (SD_ReadyWait() == 0xFF)
        res = RES_OK;
      break;

    case MMC_GET_CSD:
      /* CSD 정보 수신 (16 bytes) */
      if (SD_SendCmd(CMD9, 0) == 0 && SD_RxDataBlock(ptr, 16))
        res = RES_OK;
      break;

    case MMC_GET_CID:
      /* CID 정보 수신 (16 bytes) */
      if (SD_SendCmd(CMD10, 0) == 0 && SD_RxDataBlock(ptr, 16))
        res = RES_OK;
      break;

    case MMC_GET_OCR:
      /* OCR 정보 수신 (4 bytes) */
      if (SD_SendCmd(CMD58, 0) == 0) { // @suppress("No break at end of case")
      /* no break */
        for (n = 0; n < 4; n++)
        {
          *ptr++ = SPI_RxByte();
        }

        res = RES_OK;
      }
			break;
      break;

    default:
      res = RES_PARERR;
    }

    DESELECT();
    SPI_RxByte();
  }

  return res;
}

//uint8_t SD_test (SD_t* SD_data, const char* name){
//
//	uint8_t wtext[] = "This is SD card working with FatFs"; /* File write buffer */
//
//	//save file name
//	strcpy (SD_data->file_name, name);
//
//	//make all result with errors
//	SD_data->result.mount = 255;
//	SD_data->result.open = 255;
//	SD_data->result.close = 255;
//	SD_data->result.read = 255;
//	SD_data->result.write = 255;
//	SD_data->result.compare = 255;
//
//	//Register the file system object to the FatFs module
//	SD_data->result.mount = f_mount(&SD_data->file_system, "", 0);
//	if(SD_data->result.mount!=0) {
//		strcpy (debug_message, "mount error");
//		Error_Handler();
//		return SD_data->result.mount;
//	}
//
//	//Create and Open new text file objects with write access
//	SD_data->result.open = f_open(&SD_data->file, SD_data->file_name, FA_OPEN_ALWAYS| FA_WRITE);
//	if(SD_data->result.open!=0) {
//		strcpy (debug_message, "open error");
//		Error_Handler();
//		return SD_data->result.open;
//	}
//
//	//Write data to the text files
//	SD_data->result.write = f_write(&SD_data->file, wtext, sizeof(wtext), &SD_data->write_byte_counter);
//	if(SD_data->result.write!=0) {
//		strcpy (debug_message, "write error");
//		Error_Handler();
//		return SD_data->result.write;
//	}
//
//	//Close the files
//	SD_data->result.close = f_close(&SD_data->file);
//	if(SD_data->result.close!=0) {
//		strcpy (debug_message, "close error");
//		Error_Handler();
//		return SD_data->result.close;
//	}
//
//	//Open the text files object with read access
//	SD_data->result.open = f_open(&SD_data->file, SD_data->file_name, FA_READ);
//	if(SD_data->result.open!=0) {
//		strcpy (debug_message, "open error");
//		Error_Handler();
//		return SD_data->result.open;
//	}
//
//	//Read data from the text files
//	SD_data->result.read = f_read(&SD_data->file, &SD_data->ring_bufer, sizeof(wtext), &SD_data->read_byte_counter);
//	if(SD_data->result.read!=0) {
//		strcpy (debug_message, "read error");
//		Error_Handler();
//		return SD_data->result.read;
//	}
//
//	//Close the files
//	SD_data->result.close = f_close(&SD_data->file);
//	if(SD_data->result.close!=0) {
//		strcpy (debug_message, "close error");
//		Error_Handler();
//		return SD_data->result.close;
//	}
//
//	//Compare read data with the expected data
//	SD_data->result.compare = memcmp(wtext, &SD_data->ring_bufer, sizeof (wtext));
//
//	//return 0 if FatFs is working well, or
//	//return1 if FatFs is NOT working
//	return SD_data->result.compare;
//
//	//	if(SD_test(&SD, "test file.bin")){
//	//		printf("SD_test error: %s\r\n",SD.file_name);
//	//	}
//	//	printf("SD_test: %s\r\n",SD.file_name);
//	//	printf("mount %d\r\n",SD.result.mount);
//	//	printf("open %d\r\n",SD.result.open);
//	//	printf("write %d\r\n",SD.result.write);
//	//	printf("read %d\r\n",SD.result.read);
//	//	printf("close %d\r\n",SD.result.close);
//	//	printf("compare %d\r\n",SD.result.compare);
//}

uint8_t SD_read_settings (SD_t* SD_data){
	char str_buf[3];

	//deny update and play until opening file
	SD_data->update_flag=0;
	SD_data->play_flag=0;

	//Register the file system object to the FatFs module
	SD_data->result.mount = f_mount(&SD_data->file_system, "", 0);
	if(SD_data->result.mount!=0) return SD_data->result.mount;

	//Open the files object with read access
	SD_data->result.open = f_open(&SD_data->file, "settings.txt", FA_READ);
	if(SD_data->result.open!=0) return SD_data->result.open;

	// Read settings from the file
	SD_data->result.read = f_read(&SD_data->file, &SD_data->ring_bufer[0], 512, &SD_data->read_byte_counter);
	if(SD_data->result.read!=0) return SD_data->result.read;

	//Close the files object
	SD_data->result.close = f_close(&SD_data->file);
	if(SD_data->result.close!=0) return SD_data->result.close;

	memcpy(str_buf, &SD_data->ring_bufer[brightness_pos], 3);
	SD_data->brightness = atoi(str_buf);

	memcpy(str_buf, &SD_data->ring_bufer[contrast_pos], 3);
	SD_data->contrast = atoi(str_buf);

	memcpy(str_buf, &SD_data->ring_bufer[radio_channel_num_pos], 3);
	SD_data->radio_channel_num = atoi(str_buf);

	memcpy(SD_data->file_name, &SD_data->ring_bufer[file_name_pos], 3);
	strcpy (&SD_data->file_name[3], ".bin");//name mask

	memcpy(str_buf, SD_data->file_name, 3);
	SD_data->file_num = atoi(str_buf);

	memcpy(str_buf, &SD_data->ring_bufer[repeate_mode_pos], 3);
	SD_data->repeate_mode = atoi(str_buf);

	#ifdef DEBUG_ON
	printf("SETTINGS: \r\n");
	printf("brightness: %d\r\n",SD_data->brightness);
	printf("contrast: %d\r\n",SD_data->contrast);
	printf("radio channel: %d\r\n",SD_data->radio_channel_num);
	printf("file name: %s\r\n",SD_data->file_name);
	printf("repeate mode: %d\r\n",SD_data->repeate_mode);
	printf("\r\n");
	#endif

	return f_mount(&SD_data->file_system, "", 0) | f_open(&SD_data->file, SD_data->file_name, FA_READ);
}

uint8_t SD_change_file (SD_t* SD_data, int8_t direction, uint8_t trying_quantity){

	//имя текущего файла без расширения
	char str_buf[4];
	memcpy(str_buf, SD_data->file_name, 3);

	//новый номер файла = номер текущего файла + direction (+1, либо минус 1)
	SD_data->file_num = atoi(str_buf) + direction;

/*_________________________конструктор нового имени файла____________________________*/
	//в незадействованные разряды имени - символы нулей
	strcpy (str_buf, "000");

	//в задействованные разряды - номер нового файла, преобразованный в символы
	if (SD_data->file_num >= 100) itoa(SD_data->file_num, &str_buf[0], 10);
	else if (SD_data->file_num >= 10) itoa(SD_data->file_num, &str_buf[1], 10);
	else  itoa(SD_data->file_num, &str_buf[2], 10);

	//новое имя файла без расширения
	memcpy(SD_data->file_name, str_buf, 3);

	//новое имя файла с расширением
	strcpy (&SD_data->file_name[3], ".bin");//extension mask
/*__________________________________________________________________________________*/

/*_________________________если файл с новым именем существует:_____________________*/
	if(f_mount(&SD_data->file_system, "", 0) || f_open(&SD_data->file, SD_data->file_name, FA_READ) == FR_OK){

		printf("file name: %s", SD_data->file_name); printf(" open OK\r\n");

		//открыть файл с настройками
		f_mount(&SD_data->file_system, "", 0);
		SD_data->result.open = f_open(&SD_data->file, "settings.txt", FA_OPEN_ALWAYS | FA_WRITE);
		if(SD_data->result.open!=0) return SD_data->result.open;

		//найти позицию с именем файла
		SD_data->result.lseek = f_lseek(&SD_data->file, file_name_pos);
		if(SD_data->result.lseek!=0) return SD_data->result.lseek;

		//записать новое имя файла, открываемого по умолчанию
		SD_data->result.write = f_write(&SD_data->file, str_buf, 3, &SD_data->write_byte_counter);
		if(SD_data->result.write!=0) return SD_data->result.write;

		//Close the files object
		SD_data->result.close = f_close(&SD_data->file);
		if(SD_data->result.close!=0) return SD_data->result.close;

		return FR_OK;
	}
/*__________________________________________________________________________________*/

/*_________________________если файл с новым именем НЕсуществует:_____________________*/
	//попытка открыть следующий файл если количество попыток ещё не исчерпано
	else if (trying_quantity-- !=0){
		return SD_change_file (SD_data, direction, trying_quantity);
	}
	else {
		printf("failed to open file!:(\r\n");
		return !FR_OK;
	}
}

uint8_t SD_prepare_file (SD_t* SD_data){

	SD_data->ring_bufer_size=sizeof (SD_data->ring_bufer);
	SD_data->ring_bufer_half_size=SD_data->ring_bufer_size/2;
	SD_data->write_address_pointer=0;
	SD_data->read_address_pointer=3;
	SD_data->update_flag=0;

	//Register the file system object to the FatFs module
	SD_data->result.mount = f_mount(&SD_data->file_system, "", 0);
	if(SD_data->result.mount!=0) return SD_data->result.mount;

	//Open the files object with read access
	SD_data->result.open = f_open(&SD_data->file, SD_data->file_name, FA_READ);
	if(SD_data->result.open!=0) return SD_data->result.open;

	// Read settings from the file's head
	SD_data->result.read = f_read(&SD_data->file, &SD_data->file_head, 16, &SD_data->read_byte_counter);
	if(SD_data->result.read!=0) return SD_data->result.read;

	// Read first GRB dump from the files
	SD_data->result.read = f_read(&SD_data->file, &SD_data->ring_bufer[0], SD_data->ring_bufer_size, &SD_data->read_byte_counter);
	if(SD_data->result.read!=0) return SD_data->result.read;

	SD_data->total_read_byte_counter = SD_data->ring_bufer_half_size + 16;

	#ifdef DEBUG_ON
	printf("file prepare: %s\r\n",SD_data->file_name);
	printf("protocol_number: %d\r\n",SD_data->file_head.protocol_number);
	printf("bytes_quantity_in_file: %lu\r\n",SD_data->file_head.bytes_quantity_in_file);
	printf("pixels_quantity: %d\r\n",SD_data->file_head.pixels_quantity);
	printf("frames_quantity: %lu\r\n",SD_data->file_head.frames_quantity);
	printf("frame_rate: %d\r\n",SD_data->file_head.frame_rate);
	#endif

	return 0;
}

uint8_t SD_update_bufer (SD_t* SD_data){

	uint32_t time1 = HAL_GetTick();

	// Read next GRB dump from the files
	SD_data->read_byte_counter = 0;
	f_lseek(&SD_data->file, SD_data->total_read_byte_counter);
	SD_data->result.read = f_read(&SD_data->file, &SD_data->ring_bufer[SD_data->write_address_pointer], SD_data->ring_bufer_half_size, &SD_data->read_byte_counter);

	uint32_t time2 = HAL_GetTick();
	uint32_t time3 = time2 - time1;
	if (time3>20) {
		printf("%d\r\n", (int) time3);
	}

	return (SD_data->read_byte_counter ^ SD_data->ring_bufer_half_size) || SD_data->result.read;
}
