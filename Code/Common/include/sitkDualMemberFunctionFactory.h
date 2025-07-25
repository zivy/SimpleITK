/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef sitkDualMemberFunctionFactory_h
#define sitkDualMemberFunctionFactory_h

#include "sitkDetail.h"
#include "sitkMemberFunctionFactoryBase.h"


namespace itk::simple::detail
{


/** \class DualMemberFunctionFactory
 * \brief A class used to instantiate and generate function objects of
 *  templated member functions with two template arguments.
 *
 *  \tparam TMemberFunctionPointer is the type of pointer to member
 *  function
 *
 *  Example member function and pointer:
 *  \code
 *  type alias Image::Pointer (Self::*MemberFunctionType)( Image::Pointer );
 *
 *  template<typename TImageType1, TImageType2>
 *  Image::Pointer ExecuteInternal( Image::Pointer );
 *  \endcode
 *
 *  The  provided Addressor will instantiate the templated
 *  member functions by taking the address in the
 *  RegisterMethods. Later they can be retrieve with the
 *  GetMemberFunction method, which returns a function object with the
 *  same arguments as the templated member function pointer.
 *
 *  An instance of a MemberFunctionFactory is bound to a specific
 *  instance of an object, so that the returned function object does
 *  not need to have the calling object specified.
 *
 * \warning Use this class with caution because it can instantiate a
 * combinatorial number of methods.
 *
 * \sa MemberFunctionFactory
 */
template <typename TMemberFunctionPointer,
          typename TContainer = std::unordered_map<std::tuple<unsigned int, int, unsigned int, int>,
                                                   TMemberFunctionPointer,
                                                   hash<std::tuple<unsigned int, int, unsigned int, int>>>>
class DualMemberFunctionFactory
  : protected MemberFunctionFactoryBase<TMemberFunctionPointer,
                                        std::tuple<unsigned int, int, unsigned int, int>,
                                        TContainer>
{

public:
  using Superclass =
    MemberFunctionFactoryBase<TMemberFunctionPointer, std::tuple<unsigned int, int, unsigned int, int>, TContainer>;
  using Self = DualMemberFunctionFactory;

  using MemberFunctionType = TMemberFunctionPointer;
  using ObjectType = typename ::detail::FunctionTraits<MemberFunctionType>::ClassType;
  using FunctionObjectType = typename Superclass::FunctionObjectType;

  /** \brief Constructor which permanently binds the constructed
   * object to pObject */
  DualMemberFunctionFactory() = default;

  using Superclass::GetLoadFactor;
  using Superclass::GetMaximumLoadFactor;

  /** \brief Registers a specific member function.
   *
   * Registers a member function templated over TImageType1 and TImageType2 */
  template <typename TImageType1, typename TImageType2>
  void
  Register(MemberFunctionType pfunc, TImageType1 *, TImageType2 *);

  /** \brief Registers the member functions for all combinations of
   * TPixelIDTypeList1 and PixelIDTypeList2
   *
   * With out the third template argument, the DualExecuteInternalAddressor
   * will be used to instantiate "DualExecuteInternal" methods over
   * the two image types referenced by all combination of type in the
   * first list with types in the second.
   *
   * The optional third template parameter provides a custom addressor.
   *
   * Example usage:
   * \code
   * this->m_MemberFactory->RegisterMemberFunctions< PixelIDTypeList1,
   *                                                 PixelIDTypeList2, 3 > ();
   * \endcode
   *
   * or if a custom addressor is needed:
   * \code
   * template < class TMemberFunctionPointer >
   *    struct MyCustomAddressor
   *    {
   *      using ObjectType = typename ::detail::FunctionTraits<TMemberFunctionPointer>::ClassType;
   *
   *      template< typename TImageType1, typename TImageType2 >
   *      TMemberFunctionPointer operator() ( void ) const
   *      {
   *        return &ObjectType::template CustomMethod< TImageType1, TImageType2 >;
   *      }
   *    };
   *
   * this->m_MemberFactory->RegisterMemberFunctions< PixelIDTypeList1,
   *                                                 PixelIDTypeList2,
   *                                                 3,
   *                                                 MyCustomAddressor<TMFP> > ();
   * \endcode
   * @{
   */
  template <typename TPixelIDTypeList1, typename TPixelIDTypeList2, unsigned int VImageDimension, typename TAddressor>
  void
  RegisterMemberFunctions();
  template <typename TPixelIDTypeList1, typename TPixelIDTypeList2, unsigned int VImageDimension>
  void
  RegisterMemberFunctions()
  {
    using AddressorType = detail::DualExecuteInternalAddressor<MemberFunctionType>;
    this->RegisterMemberFunctions<TPixelIDTypeList1, TPixelIDTypeList2, VImageDimension, AddressorType>();
  }

  template <typename TPixelIDTypeList, unsigned int VImageDimension, typename TAddressor>
  void
  RegisterMemberFunctions();
  template <typename TPixelIDTypeList, unsigned int VImageDimension>
  void
  RegisterMemberFunctions()
  {
    using AddressorType = detail::DualExecuteInternalAddressor<MemberFunctionType>;
    this->RegisterMemberFunctions<TPixelIDTypeList, VImageDimension, AddressorType>();
  }
  /** @} */

  /** \brief Query to determine if an member function has been
   * registered for pixelID1, pixelID2 and imageDimension
   */
  bool
  HasMemberFunction(PixelIDValueType pixelID1, PixelIDValueType pixelID2, unsigned int imageDimension) const noexcept;


  /** \brief Returns a function object for the combination of
   *  PixelID1 and PixelID2, and image dimension, with a object pointer.
   *
   *  This overload of GetMemberFunction allows the caller to specify a
   *  custom object pointer type, which can be useful in certain
   *  situations where the object pointer type needs to be different from
   *  the one used during factory construction.
   *
   *  pixelID1 or pixelID2 is the value of Image::GetPixelIDValue(),
   *  or PixelIDToPixelIDValue<PixelIDType>::Result
   *
   *  imageDimension is the value returned by Image::GetDimension()
   *
   *  Example usage:
   *  \code
   *  PixelIDValueType pixelID = image->GetPixelIDValue();
   *  unsigned int dimension = image->GetDimension();
   *  return this->m_MemberFactory->GetMemberFunction( pixelID, pixelID, dimension, objectPointer )( image );
   *  \endcode
   *
   *  If the requested member function is not registered then an
   *  exception is generated. The returned function object is
   *  guaranteed to be valid.
   */
  FunctionObjectType
  GetMemberFunction(PixelIDValueType pixelID1,
                    PixelIDValueType pixelID2,
                    unsigned int     imageDimension,
                    ObjectType *     objectPointer) const;
};

} // namespace itk::simple::detail

#include "sitkDualMemberFunctionFactory.hxx"

#endif //  sitkDualMemberFunctionFactory_h
