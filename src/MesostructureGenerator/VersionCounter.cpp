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
#include "VersionCounter.h"

#include <Logger.h>

bool VersionCounter::mustUpdate(const VersionCounter& other) {
#ifndef NDEBUG
	if (other.m_version < m_version) {
		ERR_LOG
			<< "Inconsistent versions! "
			<< "New version should be at least " << m_version << " "
			<< "but " << other.m_version << " was found.";
	}
#endif // NDEBUG
	if (m_version > -1 && other.m_version <= m_version) return false;
	m_version = other.m_version;
	return true;
}

void VersionCounter::increment() {
	++m_version;
}

void VersionCounter::reset() {
	m_version = -1;
}
