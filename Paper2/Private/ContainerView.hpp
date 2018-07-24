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

        template<class CT, class TO>
        struct ContainerViewTraits<true, CT, TO>
        {
            using ContainerType = const CT;
            using IterType = typename CT::ConstIter;
            using ValueType = const TO;
            using PathType = const Path;
        };

        template<class CT, class TO>
        struct ContainerViewTraits<false, CT, TO>
        {
            using ContainerType = CT;
            using IterType = typename CT::Iter;
            using ValueType = TO;
            using PathType = Path;
        };

        template<bool Const, class CT, class TO>
        class ContainerView
        {
        public:
            using Traits = ContainerViewTraits<Const, CT, TO>;
            using ValueType = typename Traits::ValueType;
            using InternalIter = typename Traits::IterType;
            using PathType = typename Traits::PathType;
            using ContainerType = typename Traits::ContainerType;

            template<class VT>
            struct IterT
            {
                using ValueType = VT;

                bool operator == (const IterT & _other) const
                {
                    return m_it == _other.m_it;
                }

                bool operator != (const IterT & _other) const
                {
                    return m_it != _other.m_it;
                }

                ValueType operator* () const
                {
                    return ValueType(m_view->m_path, std::distance(m_view->m_container->begin(), m_it));
                }

                IterT operator++()
                {
                    m_it++;
                    return *this;
                }

                IterT operator += (int _i)
                {
                    m_it += _i;
                    return *this;
                }

                IterT operator + (int _i)
                {
                    IterT ret(*this);
                    ret.m_it += _i;
                    return ret;
                }

                IterT operator++(int)
                {
                    IterT ret(*this);
                    ++(*this);
                    return ret;
                }

                InternalIter m_it;
                const ContainerView * m_view;
            };

            using Iter = IterT<ValueType>;

            ContainerView() :
                m_path(nullptr),
                m_container(nullptr)
            {
            }

            ContainerView(PathType * _path, InternalIter _begin, ContainerType * _container) :
                m_path(_path),
                m_begin(_begin),
                m_container(_container)
            {

            }

            ContainerView(const ContainerView &) = default;
            ContainerView(ContainerView &&) = default;
            ContainerView & operator = (const ContainerView &) = default;
            ContainerView & operator = (ContainerView &&) = default;


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

            ValueType operator [](Size _idx) const
            {
                return *Iter{m_begin + _idx};
            }

        private:

            PathType * m_path;
            ContainerType * m_container;
            InternalIter m_begin;
        };
    }
}

#endif //PAPER_PRIVATE_CONTAINERVIEW_HPP
