#include <AT24Cxx.h>

#if (_EEPROM_SIZE_KBIT == 1) || (_EEPROM_SIZE_KBIT == 2)
#define _EEPROM_PSIZE     8
#elif (_EEPROM_SIZE_KBIT == 4) || (_EEPROM_SIZE_KBIT == 8) || (_EEPROM_SIZE_KBIT == 16)
#define _EEPROM_PSIZE     16
#else
#define _EEPROM_PSIZE     32
#endif

bool at24_isConnected(void)
{
  #if (_EEPROM_USE_WP_PIN==1)
  HAL_GPIO_WritePin(_EEPROM_WP_GPIO,_EEPROM_WP_PIN,GPIO_PIN_SET);
  #endif
  if (HAL_I2C_IsDeviceReady(&_EEPROM_I2C, _EEPROM_ADDRESS, 2, 100) == HAL_OK)
    return true;
  else
    return false;	
}

bool at24_eraseChip(void)
{
  uint8_t eraseData[32] = {};
  memset(eraseData, 0xFF, 32);

  uint32_t address = 0;

  #if (_EEPROM_USE_WP_PIN==1)
  HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN,GPIO_PIN_RESET);
  #endif

  while ( address < (_EEPROM_SIZE_KBIT * 128))
  {
	#if ((_EEPROM_SIZE_KBIT==1) || (_EEPROM_SIZE_KBIT==2))
	if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS, address, I2C_MEMADD_SIZE_8BIT, eraseData, _EEPROM_PSIZE, 100) == HAL_OK)
	#elif (_EEPROM_SIZE_KBIT==4)
	if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS | ((address & 0x0100) >> 7), (address & 0xff), I2C_MEMADD_SIZE_8BIT, eraseData, _EEPROM_PSIZE, 100) == HAL_OK)
	#elif (_EEPROM_SIZE_KBIT==8)
	if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS | ((address & 0x0300) >> 7), (address & 0xff), I2C_MEMADD_SIZE_8BIT, eraseData, _EEPROM_PSIZE, 100) == HAL_OK)
	#elif (_EEPROM_SIZE_KBIT==16)
	if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS | ((address & 0x0700) >> 7), (address & 0xff), I2C_MEMADD_SIZE_8BIT, eraseData, _EEPROM_PSIZE, 100) == HAL_OK)
	#else
	if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS, address, I2C_MEMADD_SIZE_16BIT, eraseData, _EEPROM_PSIZE, 100) == HAL_OK)
	#endif
	{
	  HAL_Delay(10);
	  address += _EEPROM_PSIZE;
	} else {
	  #if (_EEPROM_USE_WP_PIN==1)
	  HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN, GPIO_PIN_SET);
	  #endif
	  return false;
	}
  }

  #if (_EEPROM_USE_WP_PIN==1)
  HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN, GPIO_PIN_SET);
  #endif

  return true;
}

bool at24_write(uint16_t address, uint8_t *data, size_t len)
{
  uint16_t w;
	
  #if	(_EEPROM_USE_WP_PIN==1)
  HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN,GPIO_PIN_RESET);
  #endif
	
  while (1)
  {
    w = _EEPROM_PSIZE - (address  % _EEPROM_PSIZE);
    if (w > len)
      w = len;        
    #if ((_EEPROM_SIZE_KBIT==1) || (_EEPROM_SIZE_KBIT==2))
    if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS, address, I2C_MEMADD_SIZE_8BIT, data, w, 100) == HAL_OK)
    #elif (_EEPROM_SIZE_KBIT==4)
    if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS | ((address & 0x0100) >> 7), (address & 0xff), I2C_MEMADD_SIZE_8BIT, data, w, 100) == HAL_OK)
    #elif (_EEPROM_SIZE_KBIT==8)
    if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS | ((address & 0x0300) >> 7), (address & 0xff), I2C_MEMADD_SIZE_8BIT, data, w, 100) == HAL_OK)
    #elif (_EEPROM_SIZE_KBIT==16)
    if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS | ((address & 0x0700) >> 7), (address & 0xff), I2C_MEMADD_SIZE_8BIT, data, w, 100) == HAL_OK)
    #else
    if (HAL_I2C_Mem_Write(&_EEPROM_I2C, _EEPROM_ADDRESS, address, I2C_MEMADD_SIZE_16BIT, data, w, 100) == HAL_OK)
    #endif
    {
      HAL_Delay(10);
      len -= w;
      data += w;
      address += w;
      if (len == 0)
      {
        #if (_EEPROM_USE_WP_PIN==1)
        HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN, GPIO_PIN_SET);
        #endif
        return true;
      }

    }
    else
    {
      #if (_EEPROM_USE_WP_PIN==1)
      HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN, GPIO_PIN_SET);
      #endif
      return false;
    }
  }
}

bool at24_read(uint16_t address, uint8_t *data, size_t len)
{
  #if (_EEPROM_USE_WP_PIN==1)
  HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN, GPIO_PIN_SET);
  #endif
  #if ((_EEPROM_SIZE_KBIT==1) || (_EEPROM_SIZE_KBIT==2))
  if (HAL_I2C_Mem_Read(&_EEPROM_I2C, _EEPROM_ADDRESS, address, I2C_MEMADD_SIZE_8BIT, data, len, 100) == HAL_OK)
  #elif (_EEPROM_SIZE_KBIT == 4)
  if (HAL_I2C_Mem_Read(&_EEPROM_I2C, _EEPROM_ADDRESS | ((address & 0x0100) >> 7), (address & 0xff), I2C_MEMADD_SIZE_8BIT, data, len, 100) == HAL_OK)
  #elif (_EEPROM_SIZE_KBIT == 8)
  if (HAL_I2C_Mem_Read(&_EEPROM_I2C, _EEPROM_ADDRESS | ((address & 0x0300) >> 7), (address & 0xff), I2C_MEMADD_SIZE_8BIT, data, len, 100) == HAL_OK)
  #elif (_EEPROM_SIZE_KBIT==16)
  if (HAL_I2C_Mem_Read(&_EEPROM_I2C, _EEPROM_ADDRESS | ((address & 0x0700) >> 7), (address & 0xff), I2C_MEMADD_SIZE_8BIT, data, len, 100) == HAL_OK)
  #else
  if (HAL_I2C_Mem_Read(&_EEPROM_I2C, _EEPROM_ADDRESS, address, I2C_MEMADD_SIZE_16BIT, data, len, timeout) == HAL_OK)
  #endif
  {
    return true;
  }
  else
  {
    return false;	
  }    
}
