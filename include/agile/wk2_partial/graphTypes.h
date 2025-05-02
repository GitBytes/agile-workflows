/*===------------------------------------------------------------*- C++ -*-===
 *
 *                            The AGILE Workflows
 *
 *===----------------------------------------------------------------------===
 *
 * Copyright (c) 2025 Battelle Memorial Institute
 *
 * Battelle Memorial Institute (hereinafter Battelle) hereby grants permission
 * to any person or entity lawfully obtaining a copy of this software and
 * associated documentation files (hereinafter “the Software”) to redistribute
 * and use the Software in source and binary forms, with or without
 * modification. Such person or entity may use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and may permit
 * others to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Other than as used herein, neither the name Battelle Memorial Institute or
 *    Battelle may be used in any form whatsoever without the express written
 *    consent of Battelle.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *===----------------------------------------------------------------------===*/
#ifndef GRAPHTYPES_H_
#define GRAPHTYPES_H_

namespace agile::wk2_partial {

enum class TYPES {
  PERSON,
  FORUMEVENT,
  FORUM,
  PUBLICATION,
  TOPIC,
  PURCHASE,
  SALE,
  AUTHOR,
  INCLUDES,
  HASTOPIC,
  HASORG,
  NONE
};

enum class TRIPLES {
  PERSON_SALE_PERSON_BOMB_BATH,
  PERSON_SALE_PERSON_PRESSURE_COOKER,
  PERSON_SALE_PERSON_AMMUNITION,
  PERSON_SALE_PERSON_ELECTRONICS,
  PERSON_PURCHASE_PERSON_BOMB_BATH,
  PERSON_PURCHASE_PERSON_PRESSURE_COOKER,
  PERSON_PURCHASE_PERSON_AMMUNITION,
  PERSON_PURCHASE_PERSON_ELECTRONICS,
  PERSON_AUTHOR_FORUMEVENT,
  PERSON_AUTHOR_PUBLICATION,
  FORUM_INCLUDES_FORUMEVENT,
  FORUM_HASTOPIC_TOPIC_NYC,
  FORUMEVENT_HASTOPIC_TOPIC_BOMB,
  FORUMEVENT_HASTOPIC_TOPIC_EXPLOSION,
  FORUMEVENT_HASTOPIC_TOPIC_WILLIAMSBURG,
  FORUMEVENT_HASTOPIC_TOPIC_OUTDOORS,
  FORUMEVENT_HASTOPIC_TOPIC_PROSPECT_PARK,
  PUBLICATION_HASORG_TOPIC_NEAR_NYC,
  PUBLICATION_HASTOPIC_TOPIC_ELECTRICAL_ENG,
  NONE
};

constexpr uint64_t NUMTYPES = (uint64_t) TYPES::NONE;
constexpr uint64_t NUMTRIPLES = (uint64_t) TRIPLES::NONE;
using Triples = uint64_t [NUMTRIPLES];

} // namespace agile::wk2_partial

#endif // GRAPHTYPES_H
