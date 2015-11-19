// Copyright (C) 2015  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_DNN_CuDNN_H_
#define DLIB_DNN_CuDNN_H_

#ifdef DLIB_USE_CUDA

#include "cuda_errors.h"

namespace dlib
{
    class tensor;
    class resizable_tensor;

    namespace cuda 
    {

    // -----------------------------------------------------------------------------------

        class tensor_descriptor
        {
            /*!
                Each tensor object will carry a tensor_descriptor in it when compiled with
                CUDA.
            !*/

        public:
            // not copyable
            tensor_descriptor(const tensor_descriptor&) = delete;
            tensor_descriptor& operator=(const tensor_descriptor&) = delete;
            // but is movable
            tensor_descriptor(tensor_descriptor&& item) : tensor_descriptor() { swap(item); }
            tensor_descriptor& operator=(tensor_descriptor&& item) { swap(item); return *this; }

            tensor_descriptor();
            ~tensor_descriptor();

            void set_size(
                int n, 
                int k,
                int nr, 
                int nc 
            );
            /*!
                ensures
                    - if any of the arguments are 0 then they are all set to 0 in the tensor.
            !*/

            void get_size (
                int& n, 
                int& k,
                int& nr,
                int& nc 
            ) const;

            const void* get_handle (
            ) const { return handle; }

        private:

            void swap(tensor_descriptor& item) { std::swap(handle, item.handle); }

            void* handle;
        };

    // ------------------------------------------------------------------------------------

        void add(
            float beta,
            tensor& dest,
            float alpha,
            const tensor& src
        );
        /*!
            requires
                - dest.num_samples()==src.num_samples() || src.num_samples()==1
                - dest.nr()==src.nr() || src.nr()==1
                - dest.nc()==src.nc() || src.nc()==1
                - dest.k()==src.k()   || src.k()==1
                - is_same_object(src,dest) == false
            ensures
                - performs: dest = beta*dest + alpha*src
                  However, how the addition happens depends on the dimensions of src.  In
                  particular, this function adds the scaled values of one src tensor to
                  dest. Each dimension of the src tensor must match the corresponding
                  dimension of the dest tensor or must be equal to 1. In the latter case,
                  the same value from the src tensor, for those dimensions, will be used to
                  add into the dest tensor.
        !*/

        void set_tensor (
            tensor& t,
            float value
        );
        /*!
            ensures
                - sets all elements in t equal to value.
        !*/

        void scale_tensor (
            tensor& t,
            float value
        );
        /*!
            ensures
                - scales all elements of t by the given value.  I.e. for all elements E in
                  t, this function performs:
                    - E = E*value
        !*/

    // ------------------------------------------------------------------------------------

        void add_conv_bias_gradient (
            tensor& grad,
            const tensor& gradient_input
        );
        /*!
            requires
                - grad.num_samples() == 1
                - grad.k()  >= 1
                - grad.nr() == 1
                - grad.nc() == 1
                - gradient_input.k() == grad.k()
                - gradient_input.size() > 0
                - is_same_object(grad,gradient_input) == false
            ensures
                - let BIAS be a tensor with all dimensions equal to 1 except for k which is >= 1.
                - let OUT be the output of add(1,OUT,1,BIAS)
                - let f(gradient_input,BIAS) == dot(gradient_input,OUT)
                - Then this function computes the gradient of f() with respect to BIAS and
                  assigns it to grad.
        !*/

    // ------------------------------------------------------------------------------------

        class tensor_conv
        {
        public:
            tensor_conv(const tensor_conv&) = delete;
            tensor_conv& operator=(const tensor_conv&) = delete;

            tensor_conv();

            void clear(
            );

            void setup(
                const tensor& data,
                const tensor& filters,
                int stride_y,
                int stride_x
            );
            /*!
                requires
                    - filters.k() == data.k()
                    - stride_y > 0
                    - stride_x > 0
            !*/

            ~tensor_conv (
            );

            void operator() (
                resizable_tensor& output,
                const tensor& data,
                const tensor& filters
            );
            /*!
                requires
                    - The dimensions of data and filters are the same as the ones given 
                      to the last call to setup().
                    - is_same_object(output,data) == false
                    - is_same_object(output,filters) == false
                ensures
                    - convolves filters over data.  
                    - filters contains filters.num_samples() filters. 
                    - #output.num_samples() == data.num_samples()
                    - #output.k() == filters.num_samples()
                    - #output.nr() == 1+(data.nr()-1)/stride_y
                    - #output.nc() == 1+(data.nc()-1)/stride_x
            !*/

            void get_gradient_for_data (
                const tensor& gradient_input, 
                const tensor& filters,
                tensor& data_gradient
            );
            /*!
                requires
                    - filters has the same dimensions as the filters object give to the 
                      last call to setup().
                    - data_gradient has the same dimensions as the data object give to the
                      last call to setup().
                    - gradient_input has the same dimensions as the output of operator().
                    - is_same_object(data_gradient,filters) == false
                    - is_same_object(data_gradient,gradient_input) == false
                ensures
                    - let OUT be the output of (*this)(OUT,data,filters).
                    - let f(data,filters) == dot(OUT, gradient_input)
                    - This function finds the gradient of f() with respect to data
                      and adds this gradient to data_gradient.
            !*/

            void get_gradient_for_filters (
                const tensor& gradient_input, 
                const tensor& data,
                tensor& filters_gradient
            );
            /*!
                requires
                    - filters_gradient has the same dimensions as the filters object give
                      to the last call to setup().
                    - data has the same dimensions as the data object give to the last call
                      to setup().
                    - gradient_input has the same dimensions as the output of operator().
                    - is_same_object(filters_gradient,data) == false
                    - is_same_object(filters_gradient,gradient_input) == false
                ensures
                    - let OUT be the output of (*this)(OUT,data,filters).
                    - let f(data,filters) == dot(OUT, gradient_input)
                    - This function finds the gradient of f() with respect to filters 
                      and assigns this gradient to filters_gradient.
            !*/

        private:
            void* filter_handle;
            void* conv_handle;
            int stride_y;
            int stride_x;

            // dimensions of the output tensor from operator()
            int out_num_samples;
            int out_k;
            int out_nr;
            int out_nc;

            int forward_algo;
            size_t forward_workspace_size_in_bytes;
            void* forward_workspace;

            int backward_data_algo;
            size_t backward_data_workspace_size_in_bytes;
            void* backward_data_workspace;

            int backward_filters_algo;
            size_t backward_filters_workspace_size_in_bytes;
            void* backward_filters_workspace;
        };

    // ------------------------------------------------------------------------------------

        class max_pool
        {
            /*!
            !*/
        public:

            max_pool(const max_pool&) = delete;
            max_pool& operator=(const max_pool&) = delete;

            max_pool (
            );

            ~max_pool(
            );

            void clear(
            );

            void setup(
                int window_height,
                int window_width,
                int stride_y,
                int stride_x
            );

            void operator() (
                resizable_tensor& dest,
                const tensor& src
            );
            /*!
                requires
                    - is_same_object(dest,src) == false
                    - src.nr() >= stride_y
                    - src.nc() >= stride_x
                ensures
                    - #dest.num_samples() == src.num_samples()
                    - #dest.k() == src.k()
                    - #dest.nr() == src.nr()/stride_y
                    - #dest.nc() == src.nc()/stride_x
                    - for all valid s, k, r, and c:
                        - image_plane(#dest,s,k)(r,c) == max(subm_clipped(image_plane(src,s,k),
                                                                          r*stride_y,
                                                                          c*stride_x,
                                                                          window_height,
                                                                          window_width))
            !*/

            void get_gradient(
                const tensor& gradient_input, 
                const tensor& dest,
                const tensor& src,
                tensor& grad 
            );
            /*!
                requires
                    - have_same_dimensions(gradient_input,dest) == true
                    - have_same_dimensions(src,grad) == true
                    - dest contains the result of calling (*this)(dest,src)
                    - is_same_object(grad,gradient_input) == false
                    - is_same_object(grad,dest) == false
                    - is_same_object(grad,src) == false
                ensures
                    - Recalling that dest is the output of (*this)(dest,src),
                      let f(src) == dot(gradient_input,dest)
                    - Then this function computes the gradient of f() with respect to src
                      and adds it to grad.
            !*/

        private:
            void* handle;
            int stride_y;
            int stride_x;
        };

    // ------------------------------------------------------------------------------------

        void softmax (
            tensor& dest,
            const tensor& src
        );
        /*!
            requires
                - have_same_dimensions(dest, src) == true
            ensures
                - Note that the softmax function is a vector valued function: 
                    s(x) == exp(x)/sum(exp(x)) 
                - Computes the softmax function on src and writes the results to dest.  The
                  softmax is computed per spatial location across the different channels at
                  each location.  That is, softmax() outputs a new tensor, #dest, where
                  each of the spatial locations in dest (i.e. image idx, row idx, and
                  column idx) contains the output of s() evaluated over the channel values
                  at each location.
                - This function supports in-place operation, i.e. having
                  is_same_object(dest, src)==true
        !*/

        void softmax_gradient (
            tensor& grad,
            const tensor& dest,
            const tensor& gradient_input
        );
        /*!
            requires
                - have_same_dimensions(dest,gradient_input) == true 
                - have_same_dimensions(dest,grad) == true 
                - is_same_object(grad, dest)==false
            ensures
                - We interpret dest as the output of softmax(dest,SRC) for some SRC tensor.
                  Then let f(SRC) == dot(gradient_input,dest) Then this function computes
                  the gradient of f() with respect to SRC and assigns it to grad.
                - This function supports in-place operation, i.e. having
                  is_same_object(grad, gradient_input)==true
        !*/

    // ------------------------------------------------------------------------------------

        void sigmoid (
            tensor& dest,
            const tensor& src
        );
        /*!
            requires
                - have_same_dimensions(dest, src) == true
            ensures
                - for all valid i:
                    - #dest.host()[i] == 1/(1+std::exp(-src.host()[i])) 
                - This function supports in-place operation, i.e. having
                  is_same_object(dest, src)==true
        !*/

        void sigmoid_gradient (
            tensor& grad,
            const tensor& dest,
            const tensor& gradient_input
        );
        /*!
            requires
                - have_same_dimensions(dest,gradient_input) == true 
                - have_same_dimensions(dest,grad) == true 
                - is_same_object(grad,dest) == false
            ensures
                - Recalling that dest is the output of sigmoid(dest,SRC) for some SRC tensor,
                  let f(SRC) == dot(gradient_input,dest)
                - Then this function computes the gradient of f() with respect to SRC and
                  assigns it to grad.
                - This function supports in-place operation, i.e. having
                  is_same_object(grad, gradient_input)==true
        !*/

    // ------------------------------------------------------------------------------------

        void relu (
            tensor& dest,
            const tensor& src
        );
        /*!
            requires
                - have_same_dimensions(dest, src) == true
            ensures
                - for all valid i:
                    - #dest.host()[i] == std::max(0,src.host()[i]) 
                - This function supports in-place operation, i.e. having
                  is_same_object(dest, src)==true
        !*/

        void relu_gradient (
            tensor& grad,
            const tensor& dest,
            const tensor& gradient_input
        );
        /*!
            requires
                - have_same_dimensions(dest,gradient_input) == true 
                - have_same_dimensions(dest,grad) == true 
                - is_same_object(grad,dest) == false
            ensures
                - Recalling that dest is the output of relu(dest,SRC) for some SRC tensor,
                  let f(SRC) == dot(gradient_input,dest)
                - Then this function computes the gradient of f() with respect to SRC and
                  assigns it to grad.
                - This function supports in-place operation, i.e. having
                  is_same_object(grad, gradient_input)==true
        !*/

    // ------------------------------------------------------------------------------------

        void tanh (
            tensor& dest,
            const tensor& src
        );
        /*!
            requires
                - have_same_dimensions(dest, src) == true
            ensures
                - for all valid i:
                    - #dest.host()[i] == std::tanh(src.host()[i]) 
                - This function supports in-place operation, i.e. having
                  is_same_object(dest, src)==true
        !*/

        void tanh_gradient (
            tensor& grad,
            const tensor& dest,
            const tensor& gradient_input
        );
        /*!
            requires
                - have_same_dimensions(dest,gradient_input) == true 
                - have_same_dimensions(dest,grad) == true 
                - is_same_object(grad,dest) == false
            ensures
                - Recalling that dest is the output of tanh(dest,SRC) for some SRC tensor,
                  let f(SRC) == dot(gradient_input,dest)
                - Then this function computes the gradient of f() with respect to SRC and
                  assigns it to grad.
                - This function supports in-place operation, i.e. having
                  is_same_object(grad, gradient_input)==true
        !*/

    // ------------------------------------------------------------------------------------

    } 
}

#endif // DLIB_USE_CUDA

#endif // DLIB_DNN_CuDNN_H_
