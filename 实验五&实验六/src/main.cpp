#include <seal/seal.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using seal::Ciphertext;
using seal::CKKSEncoder;
using seal::Decryptor;
using seal::EncryptionParameters;
using seal::Encryptor;
using seal::Evaluator;
using seal::GaloisKeys;
using seal::KeyGenerator;
using seal::Plaintext;
using seal::PublicKey;
using seal::SEALContext;
using seal::SecretKey;
using seal::scheme_type;

constexpr std::size_t kInputRows = 4;
constexpr std::size_t kInputCols = 4;
constexpr std::size_t kKernelRows = 3;
constexpr std::size_t kKernelCols = 3;
constexpr std::size_t kOutputRows = 2;
constexpr std::size_t kOutputCols = 2;
constexpr std::size_t kInputSize = kInputRows * kInputCols;
constexpr std::size_t kKernelSize = kKernelRows * kKernelCols;
constexpr std::size_t kOutputSize = kOutputRows * kOutputCols;
constexpr std::size_t kRandomTests = 5;
constexpr std::size_t kBenchmarkRepeats = 5;
constexpr double kTolerance = 1e-4;

const std::array<int, kKernelSize> kOffsets{0, 1, 2, 4, 5, 6, 8, 9, 10};
const std::array<std::size_t, kOutputSize> kOutputSlots{0, 1, 4, 5};

struct EncodedMasks {
    std::array<Plaintext, kKernelSize> direct;
    std::array<std::array<Plaintext, kKernelCols>, kKernelRows> bsgs;
};

struct FheResult {
    Ciphertext ciphertext;
    std::size_t rotations{};
};

std::vector<double> plain_correlation(
    const std::vector<double> &input,
    const std::vector<double> &kernel)
{
    if (input.size() != kInputSize || kernel.size() != kKernelSize) {
        throw std::invalid_argument("input or kernel size is incorrect");
    }

    std::vector<double> output(kOutputSize, 0.0);
    for (std::size_t out_r = 0; out_r < kOutputRows; ++out_r) {
        for (std::size_t out_c = 0; out_c < kOutputCols; ++out_c) {
            double sum = 0.0;
            for (std::size_t kr = 0; kr < kKernelRows; ++kr) {
                for (std::size_t kc = 0; kc < kKernelCols; ++kc) {
                    const std::size_t input_index =
                        (out_r + kr) * kInputCols + (out_c + kc);
                    const std::size_t kernel_index = kr * kKernelCols + kc;
                    sum += input[input_index] * kernel[kernel_index];
                }
            }
            output[out_r * kOutputCols + out_c] = sum;
        }
    }
    return output;
}

void print_matrix(
    const std::string &title,
    const std::vector<double> &values,
    std::size_t rows,
    std::size_t cols)
{
    std::cout << title << '\n';
    std::cout << std::fixed << std::setprecision(6);
    for (std::size_t r = 0; r < rows; ++r) {
        std::cout << "  ";
        for (std::size_t c = 0; c < cols; ++c) {
            std::cout << std::setw(12) << values[r * cols + c];
        }
        std::cout << '\n';
    }
}

std::vector<double> extract_output(const std::vector<double> &slots)
{
    std::vector<double> output;
    output.reserve(kOutputSize);
    for (const std::size_t slot : kOutputSlots) {
        if (slot >= slots.size()) {
            throw std::runtime_error("decoded CKKS vector is too short");
        }
        output.push_back(slots[slot]);
    }
    return output;
}

double max_abs_error(
    const std::vector<double> &expected,
    const std::vector<double> &actual)
{
    if (expected.size() != actual.size()) {
        throw std::invalid_argument("vectors have different sizes");
    }

    double result = 0.0;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        result = std::max(result, std::abs(expected[i] - actual[i]));
    }
    return result;
}

void counted_rotate(
    Evaluator &evaluator,
    const Ciphertext &source,
    int logical_left_step,
    int rotation_sign,
    const GaloisKeys &galois_keys,
    Ciphertext &destination,
    std::size_t &counter)
{
    evaluator.rotate_vector(
        source,
        rotation_sign * logical_left_step,
        galois_keys,
        destination);
    ++counter;
}

int detect_rotation_sign(
    CKKSEncoder &encoder,
    Encryptor &encryptor,
    Evaluator &evaluator,
    Decryptor &decryptor,
    const GaloisKeys &galois_keys,
    double scale)
{
    std::vector<double> probe(encoder.slot_count(), 0.0);
    probe[0] = 1.0;
    probe[1] = 2.0;
    probe[2] = 3.0;

    Plaintext probe_plain;
    encoder.encode(probe, scale, probe_plain);

    Ciphertext probe_cipher;
    encryptor.encrypt(probe_plain, probe_cipher);

    Ciphertext rotated;
    evaluator.rotate_vector(probe_cipher, 1, galois_keys, rotated);

    Plaintext decrypted;
    decryptor.decrypt(rotated, decrypted);

    std::vector<double> decoded;
    encoder.decode(decrypted, decoded);

    if (decoded.size() >= 2 && std::abs(decoded[0] - 2.0) < 0.1) {
        return 1;
    }
    if (decoded.size() >= 2 && std::abs(decoded[1] - 1.0) < 0.1) {
        return -1;
    }
    throw std::runtime_error("unable to determine SEAL rotation direction");
}

EncodedMasks encode_masks(
    const std::vector<double> &kernel,
    CKKSEncoder &encoder,
    double scale)
{
    if (kernel.size() != kKernelSize) {
        throw std::invalid_argument("kernel size is incorrect");
    }

    EncodedMasks masks;
    const std::size_t slot_count = encoder.slot_count();

    // Direct method: every rotated ciphertext contributes directly to slots
    // {0, 1, 4, 5}, which store y00, y01, y10, y11.
    for (std::size_t i = 0; i < kKernelSize; ++i) {
        std::vector<double> mask(slot_count, 0.0);
        for (const std::size_t slot : kOutputSlots) {
            mask[slot] = kernel[i];
        }
        encoder.encode(mask, scale, masks.direct[i]);
    }

    // BSGS method:
    // group a is rotated left by 4*a after its three horizontal terms are added.
    // Therefore its useful values must first be placed at output_slot + 4*a.
    for (std::size_t a = 0; a < kKernelRows; ++a) {
        for (std::size_t b = 0; b < kKernelCols; ++b) {
            std::vector<double> mask(slot_count, 0.0);
            for (const std::size_t output_slot : kOutputSlots) {
                const std::size_t pre_rotation_slot = output_slot + 4 * a;
                if (pre_rotation_slot >= slot_count) {
                    throw std::runtime_error("slot count is too small");
                }
                mask[pre_rotation_slot] = kernel[a * kKernelCols + b];
            }
            encoder.encode(mask, scale, masks.bsgs[a][b]);
        }
    }

    return masks;
}

FheResult direct_convolution(
    const Ciphertext &encrypted_input,
    const EncodedMasks &masks,
    Evaluator &evaluator,
    const GaloisKeys &galois_keys,
    int rotation_sign)
{
    FheResult output;
    bool initialized = false;

    for (std::size_t i = 0; i < kKernelSize; ++i) {
        Ciphertext term;
        if (kOffsets[i] == 0) {
            term = encrypted_input;
        } else {
            counted_rotate(
                evaluator,
                encrypted_input,
                kOffsets[i],
                rotation_sign,
                galois_keys,
                term,
                output.rotations);
        }

        evaluator.multiply_plain_inplace(term, masks.direct[i]);
        if (!initialized) {
            output.ciphertext = std::move(term);
            initialized = true;
        } else {
            evaluator.add_inplace(output.ciphertext, term);
        }
    }

    evaluator.rescale_to_next_inplace(output.ciphertext);
    return output;
}

FheResult bsgs_convolution(
    const Ciphertext &encrypted_input,
    const EncodedMasks &masks,
    Evaluator &evaluator,
    const GaloisKeys &galois_keys,
    int rotation_sign)
{
    FheResult output;

    std::array<Ciphertext, kKernelCols> baby;
    baby[0] = encrypted_input;
    counted_rotate(
        evaluator,
        encrypted_input,
        1,
        rotation_sign,
        galois_keys,
        baby[1],
        output.rotations);
    counted_rotate(
        evaluator,
        encrypted_input,
        2,
        rotation_sign,
        galois_keys,
        baby[2],
        output.rotations);

    std::array<Ciphertext, kKernelRows> groups;
    for (std::size_t a = 0; a < kKernelRows; ++a) {
        bool initialized = false;
        for (std::size_t b = 0; b < kKernelCols; ++b) {
            Ciphertext term = baby[b];
            evaluator.multiply_plain_inplace(term, masks.bsgs[a][b]);
            if (!initialized) {
                groups[a] = std::move(term);
                initialized = true;
            } else {
                evaluator.add_inplace(groups[a], term);
            }
        }
    }

    Ciphertext group1_rotated;
    Ciphertext group2_rotated;
    counted_rotate(
        evaluator,
        groups[1],
        4,
        rotation_sign,
        galois_keys,
        group1_rotated,
        output.rotations);
    counted_rotate(
        evaluator,
        groups[2],
        8,
        rotation_sign,
        galois_keys,
        group2_rotated,
        output.rotations);

    output.ciphertext = groups[0];
    evaluator.add_inplace(output.ciphertext, group1_rotated);
    evaluator.add_inplace(output.ciphertext, group2_rotated);
    evaluator.rescale_to_next_inplace(output.ciphertext);
    return output;
}

std::vector<double> decrypt_output(
    const Ciphertext &ciphertext,
    CKKSEncoder &encoder,
    Decryptor &decryptor)
{
    Plaintext plain;
    decryptor.decrypt(ciphertext, plain);

    std::vector<double> decoded;
    encoder.decode(plain, decoded);
    return extract_output(decoded);
}

Ciphertext encrypt_input(
    const std::vector<double> &input,
    CKKSEncoder &encoder,
    Encryptor &encryptor,
    double scale)
{
    std::vector<double> packed(encoder.slot_count(), 0.0);
    std::copy(input.begin(), input.end(), packed.begin());

    Plaintext plain;
    encoder.encode(packed, scale, plain);

    Ciphertext encrypted;
    encryptor.encrypt(plain, encrypted);
    return encrypted;
}

template <typename Function>
double benchmark_ms(Function &&function, std::size_t repeats)
{
    // One untimed warm-up run.
    function();

    const auto begin = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < repeats; ++i) {
        function();
    }
    const auto end = std::chrono::steady_clock::now();

    const auto elapsed =
        std::chrono::duration<double, std::milli>(end - begin).count();
    return elapsed / static_cast<double>(repeats);
}

} // namespace

int main()
{
    try {
        std::cout << "Microsoft SEAL CKKS: encrypted 4x4 input with 3x3 kernel\n";
        std::cout << "Operation is CNN-style cross-correlation (kernel is not flipped).\n\n";

        EncryptionParameters parms(scheme_type::ckks);
        constexpr std::size_t poly_modulus_degree = 8192;
        parms.set_poly_modulus_degree(poly_modulus_degree);
        parms.set_coeff_modulus(
            seal::CoeffModulus::Create(poly_modulus_degree, {60, 40, 60}));
        constexpr double scale = static_cast<double>(1ULL << 40);

        SEALContext context(parms);
        if (!context.parameters_set()) {
            throw std::runtime_error("invalid CKKS parameters");
        }

        KeyGenerator keygen(context);
        const SecretKey secret_key = keygen.secret_key();

        PublicKey public_key;
        keygen.create_public_key(public_key);

        GaloisKeys galois_keys;
        keygen.create_galois_keys(galois_keys);

        CKKSEncoder encoder(context);
        Encryptor encryptor(context, public_key);
        Evaluator evaluator(context);
        Decryptor decryptor(context, secret_key);

        const int rotation_sign = detect_rotation_sign(
            encoder,
            encryptor,
            evaluator,
            decryptor,
            galois_keys,
            scale);

        std::cout << "CKKS parameters:\n";
        std::cout << "  poly_modulus_degree = " << poly_modulus_degree << '\n';
        std::cout << "  CKKS slot_count     = " << encoder.slot_count() << '\n';
        std::cout << "  coeff_modulus bits  = {60, 40, 60}\n";
        std::cout << "  initial scale       = 2^40\n";
        std::cout << "  logical left rotate = SEAL step "
                  << (rotation_sign > 0 ? "+d" : "-d") << "\n\n";

        const std::vector<double> input{
            1, 2, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 12,
            13, 14, 15, 16};

        const std::vector<double> kernel{
            1, 2, 3,
            4, 5, 6,
            7, 8, 9};

        const std::vector<double> expected = plain_correlation(input, kernel);
        const Ciphertext encrypted_input =
            encrypt_input(input, encoder, encryptor, scale);
        const EncodedMasks masks = encode_masks(kernel, encoder, scale);

        const FheResult direct = direct_convolution(
            encrypted_input, masks, evaluator, galois_keys, rotation_sign);
        const FheResult bsgs = bsgs_convolution(
            encrypted_input, masks, evaluator, galois_keys, rotation_sign);

        const std::vector<double> direct_values =
            decrypt_output(direct.ciphertext, encoder, decryptor);
        const std::vector<double> bsgs_values =
            decrypt_output(bsgs.ciphertext, encoder, decryptor);

        print_matrix("Input X (4x4):", input, kInputRows, kInputCols);
        print_matrix("Kernel K (3x3):", kernel, kKernelRows, kKernelCols);
        print_matrix("Plain result (2x2):", expected, kOutputRows, kOutputCols);
        print_matrix("Direct FHE result (2x2):", direct_values, kOutputRows, kOutputCols);
        print_matrix("BSGS FHE result (2x2):", bsgs_values, kOutputRows, kOutputCols);

        const double direct_error = max_abs_error(expected, direct_values);
        const double bsgs_error = max_abs_error(expected, bsgs_values);
        const double methods_error = max_abs_error(direct_values, bsgs_values);

        std::cout << "\nFixed-case verification:\n";
        std::cout << "  expected result      = [[348, 393], [528, 573]]\n";
        std::cout << "  direct rotations     = " << direct.rotations << " (expected 8)\n";
        std::cout << "  BSGS rotations       = " << bsgs.rotations << " (expected 4)\n";
        std::cout << "  direct max error     = " << std::scientific << direct_error << '\n';
        std::cout << "  BSGS max error       = " << bsgs_error << '\n';
        std::cout << "  method-to-method err = " << methods_error << '\n';

        const bool fixed_pass =
            direct.rotations == 8 &&
            bsgs.rotations == 4 &&
            direct_error < kTolerance &&
            bsgs_error < kTolerance &&
            methods_error < kTolerance;
        std::cout << "  fixed test           = " << (fixed_pass ? "PASS" : "FAIL") << "\n\n";

        std::mt19937_64 random_engine(20260727ULL);
        std::uniform_real_distribution<double> distribution(-1.0, 1.0);
        std::size_t random_passes = 0;
        double worst_random_error = 0.0;

        for (std::size_t test = 0; test < kRandomTests; ++test) {
            std::vector<double> random_input(kInputSize);
            std::vector<double> random_kernel(kKernelSize);
            for (double &value : random_input) {
                value = distribution(random_engine);
            }
            for (double &value : random_kernel) {
                value = distribution(random_engine);
                if (std::abs(value) < 0.05) {
                    value = value < 0.0 ? -0.05 : 0.05;
                }
            }

            const auto random_expected =
                plain_correlation(random_input, random_kernel);
            const auto random_encrypted =
                encrypt_input(random_input, encoder, encryptor, scale);
            const auto random_masks =
                encode_masks(random_kernel, encoder, scale);

            const auto random_direct = direct_convolution(
                random_encrypted,
                random_masks,
                evaluator,
                galois_keys,
                rotation_sign);
            const auto random_bsgs = bsgs_convolution(
                random_encrypted,
                random_masks,
                evaluator,
                galois_keys,
                rotation_sign);

            const auto random_direct_values =
                decrypt_output(random_direct.ciphertext, encoder, decryptor);
            const auto random_bsgs_values =
                decrypt_output(random_bsgs.ciphertext, encoder, decryptor);

            const double error = std::max({
                max_abs_error(random_expected, random_direct_values),
                max_abs_error(random_expected, random_bsgs_values),
                max_abs_error(random_direct_values, random_bsgs_values)});
            worst_random_error = std::max(worst_random_error, error);

            const bool pass =
                random_direct.rotations == 8 &&
                random_bsgs.rotations == 4 &&
                error < kTolerance;
            random_passes += pass ? 1U : 0U;

            std::cout << "Random test " << (test + 1) << '/' << kRandomTests
                      << ": " << (pass ? "PASS" : "FAIL")
                      << ", max error = " << std::scientific << error << '\n';
        }

        std::cout << "Random tests: " << random_passes << '/' << kRandomTests
                  << " passed; worst error = " << std::scientific
                  << worst_random_error << "\n\n";

        const double direct_ms = benchmark_ms(
            [&]() {
                volatile std::size_t rotations = direct_convolution(
                    encrypted_input,
                    masks,
                    evaluator,
                    galois_keys,
                    rotation_sign)
                                                    .rotations;
                (void)rotations;
            },
            kBenchmarkRepeats);

        const double bsgs_ms = benchmark_ms(
            [&]() {
                volatile std::size_t rotations = bsgs_convolution(
                    encrypted_input,
                    masks,
                    evaluator,
                    galois_keys,
                    rotation_sign)
                                                    .rotations;
                (void)rotations;
            },
            kBenchmarkRepeats);

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Benchmark (encryption, mask encoding and decryption excluded):\n";
        std::cout << "  direct average = " << direct_ms << " ms\n";
        std::cout << "  BSGS average   = " << bsgs_ms << " ms\n";
        std::cout << "  speedup        = " << (direct_ms / bsgs_ms) << "x\n\n";

        std::cout << "Rotation-minimum argument for the two-level BSGS model:\n";
        std::cout << "  Need (r_b + 1)(r_g + 1) >= 9.\n";
        std::cout << "  Three rotations cover at most 2*3 = 6 offsets.\n";
        std::cout << "  Four rotations with r_b = r_g = 2 cover 3*3 = 9 offsets.\n";
        std::cout << "  Therefore the implemented 4-rotation BSGS schedule reaches the minimum.\n\n";

        const bool all_pass =
            fixed_pass && random_passes == kRandomTests;
        std::cout << "FINAL STATUS: " << (all_pass ? "PASS" : "FAIL") << '\n';
        return all_pass ? 0 : 1;
    } catch (const std::exception &error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
