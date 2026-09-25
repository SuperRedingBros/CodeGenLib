#ifndef CODE_GEN_TESTS_TBUFFER_H
#define CODE_GEN_TESTS_TBUFFER_H
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>
#include <string>
#include <vector>
#include <fstream>


class TBuffer {
    std::vector<int8_t> data{};
    size_t head = 0;
    uint8_t isFile: 1 = 0;
    uint8_t isLittleEndian: 1 = 1;
public:
    static void composeBuffers(TBuffer& outBuffer, const std::initializer_list<const TBuffer*>& buffers) {
        for (const TBuffer* buffer : buffers) {
            outBuffer.pushBytes(buffer);
        }
    }

    TBuffer() {
        data.reserve(1024);
    }

    void bigEndian() {
        this->isLittleEndian = 0;
    }
    void littleEndian() {
        this->isLittleEndian = 1;
    }

    explicit TBuffer(const size_t size) {
        data.resize(size);
    }

    explicit TBuffer(const std::filesystem::path& file_name) {
        std::ifstream file(file_name.c_str(), std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Error opening file\n");
        }

        file.seekg(0, std::ios::end);
        const auto file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        data.resize(file_size);
        file.read(reinterpret_cast<char*>(data.data()), file_size);
    }

    void resetHead() {head = 0;}
    void setHead(const size_t new_pos) {head = new_pos;}
    void clear() {
        data.clear();
        head = 0;
    }
    void reserve(const size_t size) {
        data.reserve(size);
    }
    void resize(const size_t size) {
        data.resize(size);
    }

    int8_t &top() {
        return data.back();
    }

    [[nodiscard]] const void *bottom() const {
        return data.data();
    }

    [[nodiscard]] size_t size() const {
        return data.size();
    }

    int8_t* operator[](const size_t index) {
        return data.data() + head + index;
    }

    const int8_t* operator[](const size_t index) const {
        return data.data() + head + index;
    }

    void pushByte(uint8_t byte);
    void pushChar(int8_t byte);

    void pushInt8(const uint8_t number) {pushByte(number);}
    void pushInt16(uint16_t number);
    void pushInt32(uint32_t number);
    void pushInt64(uint64_t number);
    void pushFloat(float number);
    void pushDouble(double number);

    void pushSize(const size_t number) {pushInt64(number);}
    template <typename T>
    void pushArray(const std::vector<T> &array, std::function<void(T, TBuffer*)> func) {
        pushSize(array.size());
        for (size_t i = 0; i < array.size(); i++) {
            const T element = array[i];
            func(element, this);
        }
    }
    template <typename K, typename V>
    void pushMap(const std::unordered_map<K, V> &map, std::function<void(K, TBuffer*)> key_func, std::function<void(V, TBuffer*)> value_func) {
        pushSize(map.size());
        for (auto& pair: map) {
            const K key = pair.first;
            const V val = pair.second;
            key_func(key, this);
            value_func(val, this);
        }
    }
    void pushString(const std::string_view string, const uint8_t sizeBytes = 8) {
        switch (sizeBytes) {
            case 8:
                pushSize(string.size());
                break;
            case 4:
                pushInt32(string.size());
                break;
            case 2:
                pushInt16(string.size());
                break;
            case 1:
                pushInt8(string.size());
                break;
            default: throw std::invalid_argument("pushString() called with bad size");
        }
        for (const char c: string) {
            pushByte(c);
        }
    }
    void pushString(const std::string& string, const uint8_t sizeBytes = 8) {
        switch (sizeBytes) {
            case 8:
                pushSize(string.size());
                break;
            case 4:
                pushInt32(string.size());
                break;
            case 2:
                pushInt16(string.size());
                break;
            case 1:
                pushInt8(string.size());
                break;
            default: throw std::invalid_argument("pushString() called with bad size");
        }
        for (const char c: string) {
            pushByte(c);
        }
    }
    void pushCString(const std::string& string) {
        for (const char c: string) {
            pushByte(c);
        }
        pushByte(0x00);
    }
    void pushBytes(const std::vector<uint8_t> &bytes) {
        pushBytes(bytes.begin(), bytes.end());
    }
    void pushBufferWithSize(const TBuffer &bytes) {
        pushBytes(&bytes);
    }
    void pushBytes(const TBuffer* bytes) {
        pushSize(bytes->size());
        pushBytes(bytes->data.begin(), bytes->data.end());
    }
    void appendBuffer(const TBuffer &bytes) {
        appendBuffer(&bytes);
    }
    void appendBuffer(const TBuffer* bytes) {
        pushBytes(bytes->data.begin(), bytes->data.end());
    }
    void pushBytes(const std::vector<int8_t>::const_iterator &begin, const std::vector<int8_t>::const_iterator &end) {
        data.insert(data.end(), begin, end);
    }
    void pushBytes(const std::vector<uint8_t>::const_iterator &begin, const std::vector<uint8_t>::const_iterator &end) {
        data.insert(data.end(), begin, end);
    }
    void setBytes(const size_t position, const std::initializer_list<uint8_t> bytes) {
        for (int i = 0; i < bytes.size(); ++i) {
            data.assign(position + i, static_cast<const int8_t>(bytes.begin()[i]));
        }
    }

    uint8_t readByte();
    int8_t readChar();
    uint8_t readInt8() {return readByte();}
    uint16_t readInt16();
    uint32_t readInt32();
    uint64_t readInt64();
    size_t readSize() {return readInt64();}
    float readFloat();
    double readDouble();
    template <typename T>
    std::vector<T> readArray(const std::function<T(TBuffer*)>& func) {
        const size_t size = readSize();
        std::vector<T> array(size);
        for (size_t i = 0; i < size; i++) {
            const T element = func(this);
            array[i] = element;
        }
        return array;
    }
    template <typename K, typename V>
    std::unordered_map<K, V> readMap(std::function<K(TBuffer*)> key_func, std::function<V(TBuffer*)> value_func) {
        const size_t size = readSize();
        std::unordered_map<K, V> map(size);
        for (size_t i = 0; i < size; i++) {
            const K key = key_func(this);
            const V value = value_func(this);
            map[key] = value;
        }
        return map;
    }
    std::string readString(const uint8_t sizeBytes = 8) {
        size_t size;
        switch (sizeBytes) {
            case 8:
                size = readSize();
                break;
            case 4:
                size = readInt32();
                break;
            case 2:
                size = readInt16();
                break;
            case 1:
                size = readByte();
                break;
            default: throw std::invalid_argument("readString() called with bad size");
        }
        std::string string;
        for (size_t i = 0; i < size; i++) {
            string.push_back(readChar());
        }
        return string;
    }
    std::string readCString() {
        std::string string;
        while (true) {
            const auto c = readChar();
            string.push_back(c);
            if (c == '\0')
                break;
        }
        return string;
    }
    std::string readLine() {
        std::string string;
        while (true) {
            const auto c = readChar();
            if (c == '\r')
                continue;
            string.push_back(c);
            if (c == '\n')
                break;
        }
        return string;
    }
    void readBufferWithSizeToBuffer(TBuffer &buffer) {
        const size_t size = readSize();

        buffer.reserve(size);
        buffer.data.insert(buffer.data.end(),
            data.begin() + static_cast<ptrdiff_t>(head),
            data.begin() + static_cast<ptrdiff_t>(head + size));
        head += size;
    }

    void exportTo(std::vector<uint8_t>& output) const {
        output.insert(output.end(), data.begin(), data.end());
    }
};


#endif //CODE_GEN_TESTS_TBUFFER_H
