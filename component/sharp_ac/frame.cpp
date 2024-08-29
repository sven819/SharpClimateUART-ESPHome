#include <cstring>
#include "propertyTypes.h"
#include "frame.h"
#include "state.h"
#include "esphome/core/log.h"

SharpFrame::SharpFrame(char c) : size(1)
{
    data = new uint8_t[size];
    data[0] = static_cast<uint8_t>(c);
}
SharpFrame::SharpFrame() : size(0)
{
}

SharpFrame::SharpFrame(const uint8_t *arr, size_t sz) : size(sz)
{
    data = new uint8_t[sz];
    memcpy(data, arr, sz);
}

SharpFrame::~SharpFrame()
{
    delete[] data;
}

SharpFrame::SharpFrame(const SharpFrame &other) : size(other.size)
{
    data = new uint8_t[size];
    memcpy(data, other.data, size);
}

uint8_t *SharpFrame::getData() const
{
    return data;
}

size_t SharpFrame::getSize() const
{
    return size;
}

int SharpFrame::setSize(size_t sz)
{
    if (this->size == 0)
    {
        this->size = sz;
        this->data = new uint8_t[this->size];
        return 1;
    }
    return 0;
}

void SharpFrame::print()
{
}

void SharpFrame::setChecksum()
{
    this->data[size - 1] = calcChecksum();
}

bool SharpFrame::validateChecksum()
{
    return this->data[size - 1] == calcChecksum();
}

uint8_t SharpFrame::calcChecksum()
{
    uint16_t sum = 0;
    for (int i = 1; i < this->size - 1; i++)
    {
        sum += this->data[i];
        sum &= 0xFF;
    }
    uint8_t checksum = (uint8_t)((256 - sum) & 0xFF);
    return checksum;
}

SharpStatusFrame::SharpStatusFrame(const uint8_t *arr) : SharpFrame(arr, 18)
{
}

int SharpStatusFrame::getTemperature()
{
    return this->data[7] & 0x0F + 16;
}

SharpModeFrame::SharpModeFrame(const uint8_t *arr) : SharpFrame(arr, 14)
{
}

int SharpModeFrame::getTemperature()
{
    if ((this->data[4] & 0xF0) == 0x10)
        return this->data[4] & 0x0F + 16;
    else
        return 16;
}

bool SharpModeFrame::getState()
{
    return (this->data[8] & 0xF0) != 0;
}

SwingVertical SharpModeFrame::getSwingVertical()
{
    return static_cast<SwingVertical>(this->data[6] & 0x0F);
}

SwingHorizontal SharpModeFrame::getSwingHorizontal()
{
    return static_cast<SwingHorizontal>((this->data[6] & 0xF0) >> 4);
}

FanMode SharpModeFrame::getFanMode()
{
    return static_cast<FanMode>((this->data[5] & 0xF0) >> 4);
}

PowerMode SharpModeFrame::getPowerMode()
{
    return static_cast<PowerMode>(this->data[5] & 0x0F);
}

bool SharpModeFrame::getIon()
{
    return (this->data[8] == 0x84);
}

SharpCommandFrame::SharpCommandFrame() : SharpFrame()
{
    this->setSize(14);

    this->data[0] = 0xdd;
    this->data[1] = 0x0b;
    this->data[2] = 0xfb;
    this->data[3] = 0x60;
    this->data[7] = 0x00;
    this->data[10] = 0x00;
    this->data[11] = 0xe4;
}

void SharpCommandFrame::setData(SharpState *state)
{
    switch (state->mode)
    {
    case PowerMode::fan:
    {
        this->data[4] = 0x01;
        break;
    }
    case PowerMode::dry:
    {
        this->data[4] = 0x00;
        break;
    }
    case PowerMode::cool:
    {
        this->data[4] = 0xC0 | (state->temperature - 15);
        break;
    }
    case PowerMode::heat:
    {
        this->data[4] = 0xC0 | (state->temperature - 15);
        break;
    }
    }

    this->data[6] = (uint8_t)state->mode;
    if (state->mode == PowerMode::fan && state->fan == FanMode::auto_fan)
        this->data[6] |= (uint8_t)FanMode::low << 4;
    else
        this->data[6] |= (uint8_t)state->fan << 4;

    if (state->state)
        this->data[5] = 0x31;
    else
        this->data[5] = 0x21;

    if (state->ion)
    {
        this->data[9] = IonMode;
        this->data[11] = 0xE4;
    }
    else
    {
        this->data[9] = 0x00;
        this->data[11] = 0x10;
    }

    this->data[8] = ((uint8_t)state->swingH << 4) | (uint8_t)state->swingV;
}

void SharpCommandFrame::setChecksum()
{
    commandChecksum();
    SharpFrame::setChecksum();
}

void SharpCommandFrame::commandChecksum()
{
    uint8_t checksum = 0x3;

    for (int i = 4; i < 12; i++)
    {
        checksum ^= this->data[i] & 0x0F;
        checksum ^= (data[i] >> 4) & 0x0F;
    }
    checksum = 0xF - (checksum & 0x0F);
    this->data[12] = (checksum << 4) | 0x01;
}

SharpACKFrame::SharpACKFrame() : SharpFrame(0x06) {}

void SharpACKFrame::setChecksum()
{
}
