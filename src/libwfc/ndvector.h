/**
 * This file is part of MesoGen, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2023).
 *   MesoGen: Designing Procedural On-Surface Stranded Mesostructures,
 *   ACM Transaction on Graphics (SIGGRAPH '23 Conference Proceedings),
 *   https://doi.org/10.1145/3588432.3591496
 *
 * Copyright (c) 2020 - 2023 -- Télécom Paris (Élie Michel <emichel@adobe.com>)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */
#pragma once

#include <vector>
#include <utility>

namespace libwfc {

/**
 * A simple multi dimensional vector, losely inspired by numpy ndarray
 */
template <typename T, int D>
class ndvector {
public:
    template<typename... Ints>
    ndvector(Ints&& ... shape)
        : m_shape{ std::forward<Ints>(shape)... }
    {
        static_assert(sizeof...(Ints) == D, "You must provide as many dimensions as there are arguments.");
        m_strides[0] = 1;
        for (int i = 1; i < D; ++i) {
            m_strides[i] = m_strides[i - 1] * m_shape[i - 1];
        }
        m_data.resize(m_strides[D - 1] * m_shape[D - 1]);
    }

    int shape(int dimension) const {
        return m_shape[dimension];
    }

    int stride(int dimension) const {
        return m_strides[dimension];
    }

    template<typename... Ints, typename Q = T>
    typename std::enable_if< !std::is_same<Q, bool>::value, T&>::type
        /* T& */ at(Ints&& ... indices) {
        static_assert(sizeof...(Ints) == D, "You must provide as many indices as there are dimensions.");
        const std::array<int, D> uncurried_indices{ std::forward<Ints>(indices)... };
        int index = 0;
        for (int i = 0; i < D; ++i) {
            index += m_strides[i] * uncurried_indices[i];
        }
        return m_data.at(index);
    }

    template<typename... Ints, typename Q = T>
    typename std::enable_if< !std::is_same<Q, bool>::value, const T&>::type
        /* const T& */ at(Ints&& ... indices) const {
        static_assert(sizeof...(Ints) == D, "You must provide as many indices as there are dimensions.");
        const std::array<int, D> uncurried_indices{ std::forward<Ints>(indices)... };
        int index = 0;
        for (int i = 0; i < D; ++i) {
            index += m_strides[i] * uncurried_indices[i];
        }
        return m_data.at(index);
    }

    // Same, but that copies the returned value, to be used when T=bool
    template<typename... Ints>
    T get_at(Ints&& ... indices) const {
        static_assert(sizeof...(Ints) == D, "You must provide as many indices as there are dimensions.");
        const std::array<int, D> uncurried_indices{ std::forward<Ints>(indices)... };
        int index = 0;
        for (int i = 0; i < D; ++i) {
            index += m_strides[i] * uncurried_indices[i];
        }
        return m_data.at(index);
    }

    template<typename... Ints>
    void set_at(const T& value, Ints&& ... indices) {
        static_assert(sizeof...(Ints) == D, "You must provide as many indices as there are dimensions.");
        const std::array<int, D> uncurried_indices{ std::forward<Ints>(indices)... };
        int index = 0;
        for (int i = 0; i < D; ++i) {
            index += m_strides[i] * uncurried_indices[i];
        }
        m_data.at(index) = value;
    }

private:
    std::vector<T> m_data;
    std::array<int, D> m_shape;
    std::array<int, D> m_strides;
};

} // namespace libwfc

