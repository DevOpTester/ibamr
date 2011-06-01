// Filename: LNodeIndex.h
// Created on 28 Feb 2004 by Boyce Griffith
//
// Copyright (c) 2002-2010, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of New York University nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef included_LNodeIndex
#define included_LNodeIndex

/////////////////////////////// INCLUDES /////////////////////////////////////

// SAMRAI INCLUDES
#include <Index.h>
#include <tbox/AbstractStream.h>
#include <tbox/DescribedClass.h>
#include <tbox/Pointer.h>

// BLITZ++ INCLUDES
#include <blitz/array.h>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBTK
{
/*!
 * \brief Class LNodeIndex provides Lagrangian and <A
 * HREF="http://www-unix.mcs.anl.gov/petsc">PETSc</A> indexing information for a
 * single node of a Lagrangian mesh.
 */
class LNodeIndex
    : public SAMRAI::tbox::DescribedClass
{
public:
    /*!
     * \brief Default constructor.
     */
    LNodeIndex(
        const int lagrangian_nidx=-1,
        const int global_petsc_nidx=-1,
        const int local_petsc_nidx=-1,
        const SAMRAI::hier::IntVector<NDIM>& periodic_offset=SAMRAI::hier::IntVector<NDIM>(0),
        const blitz::TinyVector<double,NDIM>& periodic_displacement=blitz::TinyVector<double,NDIM>(0.0));

    /*!
     * \brief Copy constructor.
     *
     * \param from The value to copy to this object.
     */
    LNodeIndex(
        const LNodeIndex& from);

    /*!
     * \brief Constructor that unpacks data from an input stream.
     */
    LNodeIndex(
        SAMRAI::tbox::AbstractStream& stream,
        const SAMRAI::hier::IntVector<NDIM>& offset);

    /*!
     * \brief Virtual destructor.
     */
    virtual
    ~LNodeIndex();

    /*!
     * \brief Assignment operator.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    LNodeIndex&
    operator=(
        const LNodeIndex& that);

    /*!
     * \return The Lagrangian index referenced by this LNodeIndex object.
     */
    int
    getLagrangianIndex() const;

    /*!
     * \brief Reset the Lagrangian index referenced by this LNodeIndex object.
     */
    void
    setLagrangianIndex(
        const int lagrangian_nidx);

    /*!
     * \return The global PETSc index referenced by this LNodeIndex object.
     */
    int
    getGlobalPETScIndex() const;

    /*!
     * \brief Reset the global PETSc index referenced by this LNodeIndex object.
     */
    void
    setGlobalPETScIndex(
        const int global_petsc_nidx);

    /*!
     * \return The local PETSc index referenced by this LNodeIndex object.
     */
    int
    getLocalPETScIndex() const;

    /*!
     * \brief Reset the local PETSc index referenced by this LNodeIndex object.
     */
    void
    setLocalPETScIndex(
        const int local_petsc_nidx);

    /*!
     * \brief Indicate that the LNodeIndex object has been shifted across a
     * periodic boundary.
     */
    virtual void
    registerPeriodicShift(
        const SAMRAI::hier::IntVector<NDIM>& offset,
        const blitz::TinyVector<double,NDIM>& displacement);

    /*!
     * \brief Get the periodic offset.
     */
    virtual const SAMRAI::hier::IntVector<NDIM>&
    getPeriodicOffset() const;

    /*!
     * \brief Get the periodic displacement.
     */
    virtual const blitz::TinyVector<double,NDIM>&
    getPeriodicDisplacement() const;

    /*!
     * \brief Copy data from the source.
     *
     * \note The cell index of the destination object is src_index + src_offset.
     */
    virtual void
    copySourceItem(
        const SAMRAI::hier::Index<NDIM>& src_index,
        const SAMRAI::hier::IntVector<NDIM>& src_offset,
        const LNodeIndex& src_item);

    /*!
     * \brief Return an upper bound on the amount of space required to pack the
     * object to a buffer.
     */
    virtual size_t
    getDataStreamSize() const;

    /*!
     * \brief Pack data into the output stream.
     */
    virtual void
    packStream(
        SAMRAI::tbox::AbstractStream& stream);

    /*!
     * \brief Unpack data from the input stream.
     */
    virtual void
    unpackStream(
        SAMRAI::tbox::AbstractStream& stream,
        const SAMRAI::hier::IntVector<NDIM>& offset);

private:
    /*!
     * Assign that to this.
     */
    void
    assignThatToThis(
        const LNodeIndex& that);

    int d_lagrangian_nidx;   // the fixed global Lagrangian index
    int d_global_petsc_nidx; // the global PETSc index
    int d_local_petsc_nidx;  // the local PETSc index

    // the periodic offset and displacement
    SAMRAI::hier::IntVector<NDIM> d_offset;
    blitz::TinyVector<double,NDIM> d_displacement;
};

/*!
 * \brief Comparison functor to order on the physical location of the Lagrangian
 * node.
 */
class LNodeIndexPosnComp
    : std::binary_function<const LNodeIndex&,const LNodeIndex&,bool>,
      std::binary_function<const LNodeIndex* const,const LNodeIndex* const,bool>
{
public:
    LNodeIndexPosnComp(
        const blitz::Array<double,2>& X_ghosted_local_form_array)
        : d_X_ghosted_local_form_array(&X_ghosted_local_form_array)
        {
            // intentionally blank
            return;
        }

    ~LNodeIndexPosnComp()
        {
            // intentionally blank
            return;
        }

    inline bool
    operator()(
        const LNodeIndex& lhs,
        const LNodeIndex& rhs)
        {
#ifdef DEBUG_CHECK_ASSERTIONS
#if ((NDIM > 3) || (NDIM < 1))
            TBOX_ERROR("operator<(const LNodeIndex&,const LNodeIndex&): not implemented for NDIM=="
                       << NDIM << endl);
#endif
#endif
            const double* const X_lhs = &(*d_X_ghosted_local_form_array)(lhs.getLocalPETScIndex(),0);
            const double* const X_rhs = &(*d_X_ghosted_local_form_array)(rhs.getLocalPETScIndex(),0);
            const bool ret_val = (
                ((X_lhs[0]< X_rhs[0])) ||
#if (NDIM > 1)
                ((X_lhs[0]==X_rhs[0])&&(X_lhs[1]< X_rhs[1])) ||
#if (NDIM > 2)
                ((X_lhs[0]==X_rhs[0])&&(X_lhs[1]==X_rhs[1])&&(X_lhs[2]< X_rhs[2])) ||
#endif
#endif
                ((X_lhs[0]==X_rhs[0])&&
#if (NDIM > 1)
                 (X_lhs[1]==X_rhs[1])&&
#if (NDIM > 2)
                 (X_lhs[2]==X_rhs[2])&&
#endif
#endif
                 (lhs.getLagrangianIndex()<rhs.getLagrangianIndex()))
                                  );
            return ret_val;
        }// operator()

    inline bool
    operator()(
        const LNodeIndex* const lhs,
        const LNodeIndex* const rhs)
        {
            return (*this)(*lhs,*rhs);
        }// operator()

private:
    const blitz::Array<double,2>* const d_X_ghosted_local_form_array;
};

/*!
 * \brief Comparison functor to order on the Lagrangian index of the Lagrangian
 * node.
 */
struct LNodeIndexLagrangianIndexComp
    : std::binary_function<const LNodeIndex&,const LNodeIndex&,bool>,
      std::binary_function<const LNodeIndex* const,const LNodeIndex* const,bool>
{
    inline bool
    operator()(
        const LNodeIndex& lhs,
        const LNodeIndex& rhs)
        {
#ifdef DEBUG_CHECK_ASSERTIONS
            TBOX_ASSERT(lhs.getLagrangianIndex() >= 0);
            TBOX_ASSERT(rhs.getLagrangianIndex() >= 0);
#endif
            return lhs.getLagrangianIndex()<rhs.getLagrangianIndex();
        }// operator()

    inline bool
    operator()(
        const LNodeIndex* const lhs,
        const LNodeIndex* const rhs)
        {
            return (*this)(*lhs,*rhs);
        }// operator()
};

/*!
 * \brief Comparison functor to order on the global PETSc index of the
 * Lagrangian node.
 */
struct LNodeIndexGlobalPETScIndexComp
    : std::binary_function<const LNodeIndex&,const LNodeIndex&,bool>,
      std::binary_function<const LNodeIndex* const,const LNodeIndex* const,bool>
{
    inline bool
    operator()(
        const LNodeIndex& lhs,
        const LNodeIndex& rhs)
        {
#ifdef DEBUG_CHECK_ASSERTIONS
            TBOX_ASSERT(lhs.getGlobalPETScIndex() >= 0);
            TBOX_ASSERT(rhs.getGlobalPETScIndex() >= 0);
#endif
            return lhs.getGlobalPETScIndex()<rhs.getGlobalPETScIndex();
        }// operator()

    inline bool
    operator()(
        const LNodeIndex* const lhs,
        const LNodeIndex* const rhs)
        {
            return (*this)(*lhs,*rhs);
        }// operator()
};

/*!
 * \brief Comparison functor to order on the local PETSc index of the
 * Lagrangian node.
 */
struct LNodeIndexLocalPETScIndexComp
    : std::binary_function<const LNodeIndex&,const LNodeIndex&,bool>,
      std::binary_function<const LNodeIndex* const,const LNodeIndex* const,bool>
{
    inline bool
    operator()(
        const LNodeIndex& lhs,
        const LNodeIndex& rhs)
        {
#ifdef DEBUG_CHECK_ASSERTIONS
            TBOX_ASSERT(lhs.getLocalPETScIndex() >= 0);
            TBOX_ASSERT(rhs.getLocalPETScIndex() >= 0);
#endif
            return lhs.getLocalPETScIndex()<rhs.getLocalPETScIndex();
        }// operator()

    inline bool
    operator()(
        const LNodeIndex* const lhs,
        const LNodeIndex* const rhs)
        {
            return (*this)(*lhs,*rhs);
        }// operator()
};

}// namespace IBTK

/////////////////////////////// INLINE ///////////////////////////////////////

#include <ibtk/LNodeIndex.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_LNodeIndex
