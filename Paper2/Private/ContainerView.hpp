#ifndef PAPER_PRIVATE_CONTAINERVIEW_HPP
#define PAPER_PRIVATE_CONTAINERVIEW_HPP

namespace paper
{
class Path;

namespace detail
{
// helper classes to neatly iterate over the SegmentData in Path
// and converting it to Segment / Curve handles along the way.
// its a shitty amount of boiler place thanks to C++ in order
// to maintain const correctness.
template <bool Const, class CT, class TO>
struct ContainerViewTraits;

template <class CT, class TO>
struct ContainerViewTraits<true, CT, TO>
{
    using ContainerType = const CT;
    using IterType = typename ContainerType::ConstIter;
    using ValueType = const TO;
    using PathType = const Path;
};

template <class CT, class TO>
struct ContainerViewTraits<false, CT, TO>
{
    using ContainerType = CT;
    using IterType = typename CT::Iter;
    using ValueType = TO;
    using PathType = Path;
};

template <bool Const, class CT, class TO>
class ContainerView
{
  public:
    using Traits = ContainerViewTraits<Const, CT, TO>;
    using ValueType = typename Traits::ValueType;
    using InternalIter = typename Traits::IterType;
    using PathType = typename Traits::PathType;
    using ContainerType = typename Traits::ContainerType;

    template <class VT>
    struct IterT
    {
        using ValueType = VT;

        // only here to satisfy IteratorTraits
        using ReferenceType = ValueType &;
        using PointerType = ValueType *;
        using TmpStorageType = typename std::remove_const<VT>::type;

        // for sol lua binding...remove this as soon as we can
        using iterator_category = std::random_access_iterator_tag;
        using value_type = ValueType;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using difference_type = std::ptrdiff_t;

        bool operator==(const IterT & _other) const
        {
            return m_it == _other.m_it;
        }

        bool operator!=(const IterT & _other) const
        {
            return m_it != _other.m_it;
        }

        // const ValueType & operator* () const
        // {
        //     //@Note: see comment at the variable declaration
        //     tmp = ValueType(m_view->m_path, std::distance(m_view->m_container->begin(), m_it));
        //     return tmp;
        // }

        ValueType operator*() const
        {
            return ValueType(m_view->m_path, std::distance(m_view->m_container->begin(), m_it));
        }

        IterT operator++()
        {
            m_it++;
            return *this;
        }

        IterT operator+=(int _i)
        {
            m_it += _i;
            return *this;
        }

        IterT operator+(int _i)
        {
            IterT ret(*this);
            ret.m_it += _i;
            return ret;
        }

        // IterT operator - (int _i)
        // {
        //     IterT ret(*this);
        //     ret.m_it -= _i;
        //     return ret;
        // }

        difference_type operator-(IterT _b)
        {
            return std::distance(_b.m_it, m_it);
        }

        IterT operator++(int)
        {
            IterT ret(*this);
            ++(*this);
            return ret;
        }

        InternalIter m_it;
        const ContainerView * m_view;
        //@NOTE This is just so we can return a reference from the * dereferencing function to
        // make things work with the sol container binding mechanics. :(
        // It might be worth it to create another proxy that deals with this rather
        // than cluttering this implementation :/
        mutable TmpStorageType tmp;
    };

    using Iter = IterT<ValueType>;

    // for sol lua binding...remove this as soon as we can
    using iterator = Iter;
    using const_iterator = Iter;
    using value_type = ValueType;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    ContainerView() : m_path(nullptr), m_container(nullptr)
    {
    }

    ContainerView(PathType * _path, InternalIter _begin, ContainerType * _container) :
        m_path(_path),
        m_container(_container),
        m_begin(_begin)
    {
    }

    ContainerView(const ContainerView &) = default;
    ContainerView(ContainerView &&) = default;
    ContainerView & operator=(const ContainerView &) = default;
    ContainerView & operator=(ContainerView &&) = default;

    Iter begin() const
    {
        return Iter{m_begin, this};
    }

    Iter end() const
    {
        return Iter{m_container->end(), this};
    }

    ValueType last() const
    {
        return ValueType(m_path, std::distance(m_begin, m_container->end()) - 1);
    }

    Size count() const
    {
        return m_container->count();
    }

    ValueType operator[](Size _idx) const
    {
        STICK_ASSERT(m_path && m_container);
        return *(Iter{m_begin + _idx, this});
    }

  private:
    PathType * m_path;
    ContainerType * m_container;
    InternalIter m_begin;
};
} // namespace detail
} // namespace paper

#endif // PAPER_PRIVATE_CONTAINERVIEW_HPP
