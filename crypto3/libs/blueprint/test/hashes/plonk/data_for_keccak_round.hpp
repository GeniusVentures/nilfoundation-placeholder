//---------------------------------------------------------------------------//
// Copyright (c) 2021-2022 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2023 Polina Chernyshova <pockvokhbtra@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_BLUEPRINT_COMPONENTS_DATA_KECCAK_ROUND_HPP
#define CRYPTO3_BLUEPRINT_COMPONENTS_DATA_KECCAK_ROUND_HPP

#include <nil/crypto3/algebra/curves/vesta.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/vesta.hpp>
#include <nil/crypto3/algebra/random_element.hpp>

#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/crypto3/hash/sha2.hpp>
#include <nil/crypto3/hash/keccak.hpp>

#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>

#include <nil/blueprint/blueprint/plonk/circuit.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

using namespace nil::crypto3;

const uint64_t RC[25] = {0, 1ULL, 0x8082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
	0x808bULL, 0x80000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
	0x8aULL, 0x88ULL, 0x80008009ULL, 0x8000000aULL,
	0x8000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
	0x8000000000008002ULL, 0x8000000000000080ULL, 0x800aULL, 0x800000008000000aULL,
	0x8000000080008081ULL, 0x8000000000008080ULL, 0x80000001ULL, 0x8000000080008008ULL};

const std::vector<std::array<uint64_t, 25>> inner_states = {
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {
        32899, 17592186044416, 32768, 1, 17592186077184,
        0, 35184374185984, 0, 35184372088832, 2097152,
        2, 512, 0, 514, 0,
        268436480, 0, 1024, 268435456, 0,
        1099511627776, 0, 1099511627780, 0, 4
    },
    {
        9236970796698600460, 4092250545529553158, 626057523912327425, 2306538108895626371, 1173341635645358336,
        1293304092434976, 1266393375296193026, 4612686711565066480, 3572814934320349200, 6918386853474468034,
        181437471070544, 17451689225912448, 14123431978033217603, 9612137362626578, 14131171423402623105,
        109225863298950544, 4469910934709993472, 291608492588557700, 4143342752895270928, 722898250671538564,
        9260980282462904729, 14339470011802853602, 37581858268459548, 4683770000893804961, 432358761588732518
    },
    {
        592319258926211651, 14940587067404002452, 6163873250186209783, 9133172271835791495, 13983250434949586883,
        10037245043040796116, 14625807227073111006, 9517639169617348992, 10802803781493464979, 1170967630360556906,
        4833658608200494670, 14411270558251773425, 10413092914151648788, 6324505867985343017, 15456637871614865798,
        15961727220218474669, 12219779720573097889, 13453918774002596887, 11249665490274026413, 16763947842887530834,
        9348458261315236693, 11269932799224724130, 5725669273397430228, 16793563075160212879, 7296601056617420707
    },
    {
        7638250137956199023, 17990125325728205105, 7906499215270811140, 10861036725959346835, 11195520138696188958,
        8358174899797462070, 8135952663530915624, 1143978644753002443, 15662404937588594201, 16535557756827863490,
        2821756897662528488, 12114361851460063201, 8845673958919045506, 13942698502943567537, 11656387723772272466,
        13322614738909770079, 2086432298574777049, 17543636310180418713, 1178364895537752846, 10832164025006223835,
        2030143342952750111, 12360607886846421348, 10479039689777663018, 16563260862735374768, 7279885679800479721
    },
    {
        4891766363406797400, 15439122233753343804, 13823342620960621853, 11746433691194652646, 4017314498112237324,
        815207819430446539, 4967747420293129338, 3818588911347179217, 12982395987346120149, 8831006501622048216,
        3273200702990303769, 11925911941096385939, 11818410238024184151, 6855937196075990472, 6813782227838587502,
        5749709705375199086, 198532287281302992, 3986921420170929948, 2084732521627207926, 3955984847012879536,
        17540298648724239738, 14973796877054370773, 9207394463793105740, 13336242423054526618, 2223831538796077986
    },
    {
        898454936699210940, 8026835929569667841, 7594412717710188589, 17691297879001667639, 12039682773981733750,
        4806751406901749727, 11830785691895369039, 6215100860000502273, 3084694277248389144, 16700214332683074198,
        1701067029580549681, 2935021215067160996, 10064659787097191500, 7604822824502759976, 1494105689337672248,
        12626178481354463734, 2395136601172298592, 4068135589652482799, 15567196270789777948, 4732526861918809121,
        2821496240805205513, 5710775155925759758, 9794593245826189275, 17281148776925903127, 7447477925633355381
    }
    //8
};

template<typename BlueprintFieldType>
auto inner_states_data(int num_round) {
    using value_type = typename BlueprintFieldType::value_type;
    using integral_type = typename BlueprintFieldType::integral_type;

    std::array<value_type, 25> prev_inner_state;
    for (std::size_t i = 0; i < 25; i++) {
        prev_inner_state[i] = value_type(integral_type(inner_states[num_round - 1][i]));
    }
    std::array<value_type, 25> inner_state;
    for (std::size_t i = 0; i < 25; i++) {
        inner_state[i] = value_type(integral_type(inner_states[num_round][i]));
    }
    value_type rc = value_type(integral_type(RC[num_round]));
    return std::make_tuple(prev_inner_state, inner_state, rc);
}

#endif // CRYPTO3_BLUEPRINT_COMPONENTS_DATA_KECCAK_ROUND_HPP
