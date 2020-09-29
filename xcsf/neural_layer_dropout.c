/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file neural_layer_dropout.c
 * @author Richard Preen <rpreen@gmail.com>
 * @copyright The Authors.
 * @date 2016--2020.
 * @brief An implementation of a dropout layer.
 */

#include "neural_layer_dropout.h"
#include "neural_activations.h"
#include "utils.h"

/**
 * @brief Allocate memory used by a dropout layer.
 * @param l The layer to be allocated memory.
 */
static void
malloc_layer_arrays(struct Layer *l)
{
    if (l->n_inputs < 1 || l->n_inputs > N_INPUTS_MAX) {
        printf("neural_layer_dropout: malloc() invalid size\n");
        l->n_inputs = 1;
        exit(EXIT_FAILURE);
    }
    l->output = calloc(l->n_inputs, sizeof(double));
    l->delta = calloc(l->n_inputs, sizeof(double));
    l->state = calloc(l->n_inputs, sizeof(double));
}

/**
 * @brief Free memory used by a dropout layer.
 * @param l The layer to be freed.
 */
static void
free_layer_arrays(const struct Layer *l)
{
    free(l->output);
    free(l->delta);
    free(l->state);
}

/**
 * @brief Creates and initialises a dropout layer.
 * @param xcsf The XCSF data structure.
 * @param n_inputs The number of inputs.
 * @param probability The probability of dropping an input.
 * @return A pointer to the new layer.
 */
struct Layer *
neural_layer_dropout_init(const struct XCSF *xcsf, const int n_inputs,
                          const double probability)
{
    (void) xcsf;
    struct Layer *l = malloc(sizeof(struct Layer));
    layer_init(l);
    l->layer_type = DROPOUT;
    l->layer_vptr = &layer_dropout_vtbl;
    l->n_inputs = n_inputs;
    l->n_outputs = n_inputs;
    l->max_outputs = n_inputs;
    l->probability = probability;
    l->scale = 1. / (1. - probability);
    malloc_layer_arrays(l);
    return l;
}

/**
 * @brief Free memory used by a dropout layer.
 * @param xcsf The XCSF data structure.
 * @param l The layer to be freed.
 */
void
neural_layer_dropout_free(const struct XCSF *xcsf, const struct Layer *l)
{
    (void) xcsf;
    free_layer_arrays(l);
}

/**
 * @brief Initialises and creates a copy of one dropout layer from another.
 * @param xcsf The XCSF data structure.
 * @param src The source layer.
 * @return A pointer to the new layer.
 */
struct Layer *
neural_layer_dropout_copy(const struct XCSF *xcsf, const struct Layer *src)
{
    (void) xcsf;
    if (src->layer_type != DROPOUT) {
        printf("neural_layer_dropout_copy(): incorrect source layer type\n");
        exit(EXIT_FAILURE);
    }
    struct Layer *l = malloc(sizeof(struct Layer));
    layer_init(l);
    l->layer_type = src->layer_type;
    l->layer_vptr = src->layer_vptr;
    l->n_inputs = src->n_inputs;
    l->n_outputs = src->n_inputs;
    l->max_outputs = src->max_outputs;
    l->probability = src->probability;
    l->scale = src->scale;
    malloc_layer_arrays(l);
    return l;
}

/**
 * @brief Dummy function since dropout layers have no weights.
 * @param xcsf The XCSF data structure.
 * @param l A dropout layer.
 */
void
neural_layer_dropout_rand(const struct XCSF *xcsf, struct Layer *l)
{
    (void) xcsf;
    (void) l;
}

/**
 * @brief Forward propagates a dropout layer.
 * @param xcsf The XCSF data structure.
 * @param l The layer to forward propagate.
 * @param input The input to the layer.
 */
void
neural_layer_dropout_forward(const struct XCSF *xcsf, const struct Layer *l,
                             const double *input)
{
    if (!xcsf->explore) {
        memcpy(l->output, input, sizeof(double) * l->n_inputs);
    } else {
        for (int i = 0; i < l->n_inputs; ++i) {
            l->state[i] = rand_uniform(0, 1);
            if (l->state[i] < l->probability) {
                l->output[i] = 0;
            } else {
                l->output[i] = input[i] * l->scale;
            }
        }
    }
}

/**
 * @brief Backward propagates a dropout layer.
 * @param xcsf The XCSF data structure.
 * @param l The layer to backward propagate.
 * @param input The input to the layer.
 * @param delta The previous layer's error (set by this function).
 */
void
neural_layer_dropout_backward(const struct XCSF *xcsf, const struct Layer *l,
                              const double *input, double *delta)
{
    (void) xcsf;
    (void) input;
    if (delta) {
        for (int i = 0; i < l->n_inputs; ++i) {
            if (l->state[i] < l->probability) {
                delta[i] = 0;
            } else {
                delta[i] += l->delta[i] * l->scale;
            }
        }
    }
}

/**
 * @brief Dummy function since a dropout layer has no weights.
 * @param xcsf The XCSF data structure.
 * @param l A dropout layer.
 */
void
neural_layer_dropout_update(const struct XCSF *xcsf, const struct Layer *l)
{
    (void) xcsf;
    (void) l;
}

/**
 * @brief Dummy function since a dropout layer cannot be mutated.
 * @param xcsf The XCSF data structure.
 * @param l A dropout layer.
 * @return False.
 */
_Bool
neural_layer_dropout_mutate(const struct XCSF *xcsf, struct Layer *l)
{
    (void) xcsf;
    (void) l;
    return false;
}

/**
 * @brief Resizes a dropout layer if the previous layer has changed size.
 * @param xcsf The XCSF data structure.
 * @param l The layer to resize.
 * @param prev The layer previous to the one being resized.
 */
void
neural_layer_dropout_resize(const struct XCSF *xcsf, struct Layer *l,
                            const struct Layer *prev)
{
    (void) xcsf;
    l->n_inputs = prev->n_outputs;
    l->n_outputs = prev->n_outputs;
    l->max_outputs = prev->n_outputs;
    l->state = realloc(l->state, sizeof(double) * l->n_outputs);
    l->output = realloc(l->output, sizeof(double) * l->n_outputs);
    l->delta = realloc(l->delta, sizeof(double) * l->n_outputs);
}

/**
 * @brief Returns the output from a dropout layer.
 * @param xcsf The XCSF data structure.
 * @param l The layer whose output to return.
 * @return The layer output.
 */
double *
neural_layer_dropout_output(const struct XCSF *xcsf, const struct Layer *l)
{
    (void) xcsf;
    return l->output;
}

/**
 * @brief Prints a dropout layer.
 * @param xcsf The XCSF data structure.
 * @param l The layer to print.
 * @param print_weights Whether to print the values of the weights and biases.
 */
void
neural_layer_dropout_print(const struct XCSF *xcsf, const struct Layer *l,
                           const _Bool print_weights)
{
    (void) xcsf;
    (void) print_weights;
    printf("dropout in = %d, out = %d prob = %f\n", l->n_inputs, l->n_outputs,
           l->probability);
}

/**
 * @brief Writes a dropout layer to a binary file.
 * @param xcsf The XCSF data structure.
 * @param l The layer to save.
 * @param fp Pointer to the file to be written.
 * @return The number of elements written.
 */
size_t
neural_layer_dropout_save(const struct XCSF *xcsf, const struct Layer *l,
                          FILE *fp)
{
    (void) xcsf;
    size_t s = 0;
    s += fwrite(&l->n_inputs, sizeof(int), 1, fp);
    s += fwrite(&l->n_outputs, sizeof(int), 1, fp);
    s += fwrite(&l->max_outputs, sizeof(int), 1, fp);
    s += fwrite(&l->probability, sizeof(double), 1, fp);
    s += fwrite(&l->scale, sizeof(double), 1, fp);
    return s;
}

/**
 * @brief Reads a dropout layer from a binary file.
 * @param xcsf The XCSF data structure.
 * @param l The layer to load.
 * @param fp Pointer to the file to be read.
 * @return The number of elements read.
 */
size_t
neural_layer_dropout_load(const struct XCSF *xcsf, struct Layer *l, FILE *fp)
{
    (void) xcsf;
    size_t s = 0;
    s += fread(&l->n_inputs, sizeof(int), 1, fp);
    s += fread(&l->n_outputs, sizeof(int), 1, fp);
    s += fread(&l->max_outputs, sizeof(int), 1, fp);
    s += fread(&l->probability, sizeof(double), 1, fp);
    s += fread(&l->scale, sizeof(double), 1, fp);
    malloc_layer_arrays(l);
    return s;
}
