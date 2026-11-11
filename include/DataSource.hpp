#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <thread>
#include <atomic>
#include "RingBuffer.hpp"

// Абстрактный источник I/Q данных.
// Фоновый поток постоянно читает "сырые" физические пакеты
// (через readRawPacket(), реализуемый наследником) и складывает
// распакованные I/Q отсчёты в потокобезопасный RingBuffer.
// readSamples() читает уже готовые данные оттуда, поэтому может
// свободно обслуживать запросы произвольного размера (numElems),
// не завися от размера одного физического UDP/MSD пакета.
class DataSource
{
public:
    DataSource();
    virtual ~DataSource();

    // Инициализация + запуск фонового потока чтения.
    bool init();

    // Остановка фонового потока (вызывается автоматически из деструктора,
    // но может быть вызвана и явно).
    void shutdown();

    // Прочитать num_samples КОМПЛЕКСНЫХ отсчётов (I+Q каждый int16_t).
    // data должен иметь размер >= num_samples*2 элементов int16_t.
    bool readSamples(int16_t *data, size_t num_samples, int timeout_ms);

    virtual bool isReady() const = 0;

protected:
    // Реализуется наследником: открыть устройство/файл/сокет.
    virtual bool openRaw() = 0;

    // Реализуется наследником: прочитать один физический пакет и
    // вернуть распакованные I/Q отсчёты (interleaved int16: I0,Q0,I1,Q1,...).
    // Возвращает false при ошибке/таймауте (не фатально - поток продолжит попытки).
    virtual bool readRawPacket(std::vector<int16_t> &out) = 0;

    virtual void closeRaw() = 0;

private:
    void threadFunc();

    std::thread _thread;
    std::atomic<bool> _running{false};
    RingBuffer _ring;
};

// ============================================================================
// MSD File Reader (режим MSD - файловый интерфейс Afedri)
// ============================================================================
class MSDDataSource : public DataSource
{
public:
    explicit MSDDataSource(const std::string &path);
    ~MSDDataSource() override;

    bool isReady() const override { return _file_handle != -1; }

protected:
    bool openRaw() override;
    bool readRawPacket(std::vector<int16_t> &out) override;
    void closeRaw() override;

private:
    std::string _path;
    int _file_handle = -1;

    static constexpr size_t READ_PACKET_SIZE = 8192;
};

// ============================================================================
// UDP Data Source (сетевой режим, данные приходят на localhost:port)
// ============================================================================
class UDPDataSource : public DataSource
{
public:
    explicit UDPDataSource(int port = 50000);
    ~UDPDataSource() override;

    bool isReady() const override { return _socket != -1; }

protected:
    bool openRaw() override;
    bool readRawPacket(std::vector<int16_t> &out) override;
    void closeRaw() override;

private:
    int _port;
    int _socket = -1;

    static constexpr size_t PACKET_SIZE = 1028;
    static constexpr size_t HEADER_SIZE = 4;
};
