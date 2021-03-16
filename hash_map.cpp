#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {

    constexpr static const size_t kEnd_ = std::numeric_limits<std::size_t>::max();
    constexpr static const double kMult_ = 1.6;

    typedef std::pair<KeyType, ValueType> ImageType;
    typedef std::pair<const KeyType, ValueType> ConstImageType;

    struct Cell {
        ConstImageType image;
        size_t prev = kEnd_, next = kEnd_;

        Cell() = default;
        Cell(const Cell& other) {
            image = other.image;
            prev = other.prev;
            next = other.next;
        }
        explicit Cell(const ImageType& image): image(image), prev(kEnd_), next(kEnd_) {}

        const KeyType& key() const {
            return image.first;
        }

        const ValueType& value() const {
            return image.second;
        }

        ValueType& value() {
            return image.second;
        }
    };

    size_t size_;
    size_t num_elements_ = 0;
    size_t head_ = kEnd_, tail_ = kEnd_;
    std::vector<Cell*> table_;
    Hash hasher_;

public:
    class iterator: public std::iterator<std::forward_iterator_tag, ConstImageType> {

        const HashMap *mp_;
        size_t id_;

    public:
        iterator(): mp_(nullptr), id_(kEnd_) {}
        iterator(const HashMap* mp, size_t id): mp_(mp), id_(id) {}
        iterator(const iterator& other): mp_(other.mp_), id_(other.id_) {}

        iterator& operator++() {
            id_ = mp_->table_[id_]->next;
            return *this;
        }

        iterator operator++(int) {
            iterator copy(*this);
            operator++();
            return copy;
        }

        ConstImageType operator*() {
            return mp_->table_[id_]->image;
        }

        ConstImageType* operator->() {
            return &mp_->table_[id_]->image;
        }

        bool operator == (const iterator& other) {
            return mp_ == other.mp_ && id_ == other.id_;
        }

        bool operator != (const iterator& other) {
            return !(*this == other);
        }
    };

    class const_iterator: public std::iterator<std::forward_iterator_tag, ConstImageType> {

        const HashMap *mp_;
        size_t id_;

    public:
        const_iterator(): mp_(nullptr), id_(kEnd_) {}
        const_iterator(const HashMap* mp, size_t id): mp_(mp), id_(id) {}
        const_iterator(const const_iterator& other): mp_(other.mp_), id_(other.id_) {}

        const_iterator& operator++() {
            id_ = mp_->table_[id_]->next;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator copy(*this);
            operator++();
            return copy;
        }

        const ConstImageType operator*() {
            return mp_->table_[id_]->image;
        }

        const ConstImageType* operator->() {
            return &mp_->table_[id_]->image;
        }

        bool operator == (const const_iterator& other) {
            return mp_ == other.mp_ && id_ == other.id_;
        }

        bool operator != (const const_iterator& other) {
            return !(*this == other);
        }
    };

    explicit HashMap(Hash hasher = Hash()): hasher_(hasher) {
        size_ = 2;
        table_.resize(TableSize(size_));
    }

    HashMap(iterator first, iterator last, Hash hasher = Hash()): hasher_(hasher) {
        size_t cnt = 1;
        for (iterator it = first; it != last; ++it) ++cnt;
        size_ = cnt * 2;
        table_.resize(TableSize(size_));
        for (iterator it = first; it != last; ++it) insert(*it);
    }

    HashMap(const_iterator first, const_iterator last, Hash hasher = Hash()): hasher_(hasher) {
        size_t cnt = 1;
        for (const_iterator it = first; it != last; ++it) ++cnt;
        size_ = cnt * 2;
        table_.resize(TableSize(size_));
        for (const_iterator it = first; it != last; ++it) insert(*it);
    }

    HashMap(std::initializer_list<ImageType> images, Hash hasher = Hash()): hasher_(hasher) {
        size_ = images.size() * 2;
        table_.resize(TableSize(size_));
        for (const ImageType* it = images.begin(); it != images.end(); ++it) insert(*it);
    }

    HashMap(const HashMap& other) {
        *this = other;
    }

    HashMap& operator=(const HashMap& other) {
        std::vector<ImageType> images;
        for (const_iterator it = other.begin(); it != other.end(); ++it)
            images.push_back(*it);
        clear();
        hasher_ = other.hasher_;
        size_ = other.size_;
        table_.resize(TableSize(size_));
        for (typename std::vector<ImageType>::iterator it = images.begin(); it != images.end(); ++it)
            insert(*it);
        return *this;
    }

    size_t size() const {
        return num_elements_;
    }

    bool empty() const {
        return num_elements_ == 0;
    }

    Hash hash_function() const {
        return hasher_;
    }

    void insert(const ImageType& image) {
        const KeyType& key = image.first;
        size_t id = GetId(key);
        if (table_[id] != nullptr) return;
        insert(image, id);
    }

    void erase(const KeyType& key) {
        size_t id = GetId(key);
        if (table_[id] == nullptr) return;
        erase(id);
        std::vector<ImageType> images;
        for (; table_[id + 1] != nullptr; ++id) {
            images.push_back(table_[id + 1]->image);
            erase(id + 1);
        }
        for (typename std::vector<ImageType>::iterator it = images.begin(); it != images.end(); ++it)
            insert(*it);
    }

    iterator begin() {
        return iterator(this, head_);
    }

    const_iterator begin() const {
        return const_iterator(this, head_);
    }

    iterator end() {
        return iterator(this, kEnd_);
    }

    const_iterator end() const {
        return const_iterator(this, kEnd_);
    }

    iterator find(const KeyType& key) {
        size_t id = GetId(key);
        return table_[id] != nullptr ? iterator(this, id) : end();
    }

    const_iterator find(const KeyType& key) const {
        size_t id = GetId(key);
        return table_[id] != nullptr ? const_iterator(this, id) : const_iterator(this, kEnd_);
    }

    ValueType& operator[](const KeyType& key) {
        insert(ImageType(key, ValueType()));
        size_t id = GetId(key);
        return table_[id]->value();
    }

    const ValueType& at(const KeyType& key) const {
        size_t id = GetId(key);
        if (table_[id] == nullptr) throw std::out_of_range("key is not found");
        return table_[id]->value();
    }

    void clear() {
        while (!empty())
            erase(head_);
    }

    ~HashMap() {
        clear();
    }

private:
    void reset(Cell*& ptr) {
        delete ptr;
        ptr = nullptr;
    }

    size_t TableSize(size_t size_) {
        return size_ * kMult_;
    }

    void Rebuild() {
        std::vector<ImageType> images;
        for (iterator it = begin(); it != end(); ++it)
            images.push_back(*it);
        clear();
        size_ *= 2;
        table_.resize(TableSize(size_));
        for (typename std::vector<ImageType>::iterator it = images.begin(); it != images.end(); ++it)
            insert(*it);
    }

    size_t GetId(const KeyType& key) const {
        size_t id = hasher_(key) % size_;
        while (table_[id] != nullptr && !(table_[id]->key() == key)) ++id;
        return id;
    }

    void insert(const ConstImageType& image, size_t id) {
        table_[id] = new Cell(image);
        if (num_elements_ == 0) {
            head_ = tail_ = id;
        } else {
            table_[tail_]->next = id;
            table_[id]->prev = tail_;
            tail_ = id;
        }
        ++num_elements_;
        if (num_elements_ * 2 > size_)
            Rebuild();
    }

    void erase(size_t id) {
        size_t prev_id = table_[id]->prev;
        size_t next_id = table_[id]->next;
        reset(table_[id]);
        if (num_elements_ != 1) {
            if (prev_id == kEnd_) {
                head_ = next_id;
                table_[next_id]->prev = kEnd_;
            } else if (next_id == kEnd_) {
                tail_ = prev_id;
                table_[prev_id]->next = kEnd_;
            } else {
                table_[prev_id]->next = next_id;
                table_[next_id]->prev = prev_id;
            }
        }
        --num_elements_;
        if (!num_elements_)
            head_ = tail_ = kEnd_;
    }
};