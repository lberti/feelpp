/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2014-02-14

  Copyright (C) 2014-2016 Feel++ Consortium

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#if !defined( FEELPP_VF_DETAIL_GINACEXVF_HPP )
#define FEELPP_VF_DETAIL_GINACEXVF_HPP 1

namespace Feel{
namespace vf{
///\cond detail
/**
 * \class Ginac
 * \brief allow runtime ginac in expression
 *
 * @author Christophe Prud'homme
 * @see
 */
template<typename ExprT,int Order=2, typename SymbolsExprType = SymbolsExpr<> >
class FEELPP_EXPORT GinacExVF : public Feel::vf::GiNaCBase
{
public:


    /** @name Typedefs
     */
    //@{
    typedef Feel::vf::GiNaCBase super;
    typedef ExprT expression_type;

    typedef SymbolsExprType symbols_expression_type;

    struct FunctorsVariadicExpr
    {
        struct Context
        {
            template <typename T1,typename T2>
            constexpr auto operator()( T1 const& res,T2 const& e ) const
                {
                    return hana::integral_constant<size_type, T1::value | T2::second_type::context >{};
                }
        };
        template<typename Funct>
        struct HasTestFunction
        {
            template <typename T1,typename T2>
            constexpr auto operator()( T1 const& res,T2 const& e ) const
                {
                    return hana::integral_constant<bool, T1::value || T2::second_type::template HasTestFunction<Funct>::result >{};
                }
        };
        template<typename Funct>
        struct HasTrialFunction
        {
            template <typename T1,typename T2>
            constexpr auto operator()( T1 const& res,T2 const& e ) const
                {
                    return hana::integral_constant<bool, T1::value || T2::second_type::template HasTrialFunction<Funct>::result >{};
                }
        };
    };
    //static const size_type context = vm::POINT|expression_type::context;
    static const size_type context =  std::decay_t<decltype( hana::fold( symbols_expression_type{}, hana::integral_constant<size_type,vm::POINT|vm::JACOBIAN|vm::KB|vm::NORMAL|expression_type::context>{}, typename FunctorsVariadicExpr::Context{} ) )>::value;

    static const bool is_terminal = false;
    static const uint16_type imorder = expression_type::imorder+Order;
    static const bool imIsPoly = false;

    template<typename Funct>
    struct HasTestFunction
    {
        //static const bool result = expression_type::template HasTestFunction<Funct>::result;
        static const bool result =  std::decay_t<decltype( hana::fold( symbols_expression_type{},
                                                                       hana::integral_constant<bool,expression_type::template HasTestFunction<Funct>::result>{},
                                                                       typename FunctorsVariadicExpr::template HasTestFunction<Funct>{} ) )>::value;
    };
    template<typename Funct>
    struct HasTrialFunction
    {
        //static const bool result = expression_type::template HasTrialFunction<Funct>::result;
        static const bool result =  std::decay_t<decltype( hana::fold( symbols_expression_type{},
                                                                       hana::integral_constant<bool,expression_type::template HasTrialFunction<Funct>::result>{},
                                                                       typename FunctorsVariadicExpr::template HasTrialFunction<Funct>{} ) )>::value;
    };
    template<typename Func>
    static const bool has_test_basis = expression_type::template has_test_basis<Func>;
    template<typename Func>
    static const bool has_trial_basis = expression_type::template has_trial_basis<Func>;
    using test_basis = typename expression_type::test_basis;
    using trial_basis = typename expression_type::trial_basis;

    typedef GiNaC::ex ginac_expression_type;
    typedef GinacExVF<ExprT,Order,SymbolsExprType> this_type;
    typedef double value_type;
    typedef value_type evaluate_type;

    typedef Eigen::Matrix<value_type,Eigen::Dynamic,1> vec_type;

    template<typename... TheExpr>
    struct Lambda
    {
        typedef this_type type;
    };
    template<typename... TheExpr>
    typename Lambda<TheExpr...>::type
    operator()( TheExpr... e  ) { return *this; }

    //@}

    /** @name Constructors, destructor
     */
    //@{

    explicit GinacExVF( ginac_expression_type const & fun,
                        std::vector<GiNaC::symbol> const& syms,
                        std::string const& exprDesc,
                        std::vector< std::pair<GiNaC::symbol, expression_type> >const& expr,
                        std::string filename="",
                        WorldComm const& world=Environment::worldComm() )
        :
        super( syms ),
        M_fun( fun ),
        M_expr( expr ),
        M_cfun( new GiNaC::FUNCP_CUBA() ),
        M_filename(),
        M_exprDesc( exprDesc )
        {
            std::string filenameExpanded = Environment::expand( filename );
            M_filename = (filenameExpanded.empty() || fs::path(filenameExpanded).is_absolute())? filenameExpanded : (fs::path(Environment::exprRepository())/filenameExpanded).string();

            DVLOG(2) << "Ginac constructor with expression_type \n";
            GiNaC::lst exprs({fun});
            GiNaC::lst syml;
            std::for_each( M_syms.begin(),M_syms.end(), [&]( GiNaC::symbol const& s ) { syml.append(s); } );

            // get filename if not given
            if ( M_filename.empty() && !M_exprDesc.empty() )
            {
                M_filename = Feel::vf::detail::ginacGetDefaultFileName( M_exprDesc );
            }

            // build ginac lib and link if necessary
            Feel::vf::detail::ginacBuildLibrary( exprs, syml, M_exprDesc, M_filename, world, M_cfun );
        }

    explicit GinacExVF( ginac_expression_type const & fun,
                        std::vector<GiNaC::symbol> const& syms,
                        GiNaC::FUNCP_CUBA const& cfun,
                        std::vector< std::pair<GiNaC::symbol, expression_type> >const& expr,
                        std::string const& exprDesc,
                        symbols_expression_type const& expr2 )
        :
        super( syms ),
        M_fun( fun ),
        M_expr( expr ),
        M_cfun( new GiNaC::FUNCP_CUBA( cfun ) ),
        M_exprDesc( exprDesc ),
        M_expr2( expr2 )
        {}

    GinacExVF( GinacExVF const & fun ) = default;
    GinacExVF( GinacExVF && fun ) = default;

    //@}

    /** @name Operator overloads
     */
    //@{

    this_type& operator=( this_type const& t )
    {
        if ( this == &t )
            return *this;
        M_fun = t.M_fun;
        M_expr = t.M_expr;
        M_cfun = t.M_cfun;
        M_exprDesc = t.M_exprDesc;
        M_expr2 = t.M_expr2;
        return *this;
    }
    //this_type& operator=( this_type && ) = default;

    //@}

    /** @name Accessors
     */
    //@{

    //@}

    /** @name  Mutators
     */
    //@{


    //@}

    /** @name  Methods
     */
    //@{

    const GiNaC::FUNCP_CUBA& fun() const
        {
            return *M_cfun;
        }
    expression_type const& expression(int i=0) const
        {
            return M_expr[i].second;
        }
    std::vector<expression_type> const expressions() const
    {
        std::vector<expression_type> exprs_vec;
        typename std::vector< std::pair<GiNaC::symbol, expression_type> >::const_iterator it = M_expr.begin();
        for(it; it!=M_expr.end(); it++)
            exprs_vec.push_back( it->second );
        return exprs_vec;
    }

    int nExpression() const { return M_expr.size(); }

    int index(int i=0) const
    {
        auto it = std::find_if( M_syms.begin(), M_syms.end(),
                                [=]( GiNaC::symbol const& s ) { return s.get_name() == M_expr[i].first.get_name(); } );
        if ( it != M_syms.end() )
            {
                return it-M_syms.begin();
            }
        return -1;
    }

    uint16_type index( std::string const& sname ) const
    {
        auto it = std::find_if( M_syms.begin(), M_syms.end(),
                                [=]( GiNaC::symbol const& s ) { return s.get_name() == sname; } );
        if ( it != M_syms.end() )
            {
                return it-M_syms.begin();
            }
        return invalid_uint16_type_value;
    }

    const std::vector<int> indices() const
    {
        std::vector<int> indices_vec;
        for(int i=0; i<M_expr.size(); i++)
            indices_vec.push_back( this->index( i ) );
        return indices_vec;
    }

    const std::vector<uint16_type> indices2() const
    {
        std::vector<uint16_type> indices_vec;
        hana::for_each( M_expr2, [&]( auto const& e )
                        {
                            indices_vec.push_back( this->index( e.first ) );
                            //std::cout << "ana::for_each " << e.first << "\n";
                        });
        return indices_vec;
    }
    bool isZero() const { return M_fun.is_zero(); }
    std::vector<GiNaC::symbol> const& syms() const { return M_syms; }


    symbols_expression_type const& symbolsExpression() const { return M_expr2; }
    //@}


    template<typename Geo_t, typename Basis_i_t, typename Basis_j_t>
    struct tensor
    {
        typedef typename expression_type::template tensor<Geo_t, Basis_i_t, Basis_j_t> tensor_expr_type;

        struct TransformExprToTensor
        {
            template <typename T>
            struct apply {
                using type = typename T::second_type::template tensor<Geo_t, Basis_i_t, Basis_j_t>;
            };

            template <typename T>
            constexpr auto operator()(T const& t) const
                {
                    return typename TransformExprToTensor::apply<T>::type( t.second,Geo_t{} );
                }
            template <typename T>
            constexpr auto operator()(T const& t, Geo_t const& geom, Basis_i_t const& fev, Basis_j_t const& feu ) const
                {
                    return typename TransformExprToTensor::apply<T>::type( t.second,geom,fev,feu );
                }
            template <typename T>
            constexpr auto operator()(T const& t, Geo_t const& geom, Basis_i_t const& fev ) const
                {
                    return typename TransformExprToTensor::apply<T>::type( t.second,geom,fev );
                }
            template <typename T>
            constexpr auto operator()(T const& t, Geo_t const& geom ) const
                {
                    return typename TransformExprToTensor::apply<T>::type( t.second,geom );
                }
        };

        using tuple_tensor_expr2_type = std::decay_t<decltype( hana::transform( symbols_expression_type{}, TransformExprToTensor{} ) ) >;

        typedef typename tensor_expr_type::value_type value_type;

        using key_type = key_t<Geo_t>;
        typedef typename fusion::result_of::value_at_key<Geo_t,key_type>::type::element_type* gmc_ptrtype;
        typedef typename fusion::result_of::value_at_key<Geo_t,key_type>::type::element_type gmc_type;
        // change 0 into rank
        typedef typename mpl::if_<mpl::equal_to<mpl::int_<0>,mpl::int_<0> >,
                                  mpl::identity<Shape<gmc_type::nDim, Scalar, false, false> >,
                                  typename mpl::if_<mpl::equal_to<mpl::int_<0>,mpl::int_<1> >,
                                                    mpl::identity<Shape<gmc_type::nDim, Vectorial, false, false> >,
                                                    mpl::identity<Shape<gmc_type::nDim, Tensor2, false, false> > >::type >::type::type shape;

        typedef Eigen::Matrix<value_type,Eigen::Dynamic,1> vec_type;

        struct is_zero
        {
            static const bool value = false;
        };

        tensor( this_type const& expr,
                Geo_t const& geom, Basis_i_t const& fev, Basis_j_t const& feu )
            :
            M_expr( expr ),
            M_fun( expr.fun() ),
            M_t_expr(std::vector<tensor_expr_type>() ),
            M_t_expr_index( std::vector<int>(expr.indices()) ),
            M_is_zero( expr.isZero() ),
            M_t_expr2( hana::transform( expr.symbolsExpression(), [&geom,&fev,&feu](auto const& t) { return TransformExprToTensor{}(t,geom,fev,feu); } ) ),
            M_t_expr2_index( expr.indices2() ),
            M_gmc( fusion::at_key<key_type>( geom ).get() ),
            M_nsyms( expr.syms().size() ),
            M_y( vec_type::Zero(M_gmc->nPoints()) ),
            M_x( expr.parameterValue() )
            {
                for (int k=0 ; k< expr.nExpression() ; ++k )
                {
                    tensor_expr_type mytensor( expr.expression(k), geom, fev, feu );
                    M_t_expr.push_back( mytensor );
                }
            }

        tensor( this_type const& expr,
                Geo_t const& geom, Basis_i_t const& fev )
            :
            M_expr( expr ),
            M_fun( expr.fun() ),
            M_t_expr(std::vector<tensor_expr_type>() ),
            M_t_expr_index( std::vector<int>(expr.indices()) ),
            M_is_zero( expr.isZero() ),
            M_t_expr2( hana::transform( expr.symbolsExpression(), [&geom,&fev](auto const& t) { return TransformExprToTensor{}(t,geom,fev); } ) ),
            M_t_expr2_index( expr.indices2() ),
            M_gmc( fusion::at_key<key_type>( geom ).get() ),
            M_nsyms( expr.syms().size() ),
            M_y( vec_type::Zero(M_gmc->nPoints()) ),
            M_x(  expr.parameterValue() )
            {
                for (int k=0 ; k< expr.nExpression() ; ++k )
                {
                    tensor_expr_type mytensor( expr.expression(k), geom, fev );
                    M_t_expr.push_back( mytensor );
                }
            }

        tensor( this_type const& expr, Geo_t const& geom )
            :
            M_expr( expr ),
            M_fun( expr.fun() ),
            M_t_expr( std::vector<tensor_expr_type>() ),
            M_t_expr_index( std::vector<int>(expr.indices()) ),
            M_is_zero( expr.isZero() ),
            M_t_expr2( hana::transform( expr.symbolsExpression(), [&geom](auto const& t) { return TransformExprToTensor{}(t,geom); } ) ),
            M_t_expr2_index( expr.indices2() ),
            M_gmc( fusion::at_key<key_type>( geom ).get() ),
            M_nsyms( expr.syms().size() ),
            M_y( vec_type::Zero(M_gmc->nPoints()) ),
            M_x( expr.parameterValue() )
            {
                for (int k=0 ; k< expr.nExpression() ; ++k )
                {
                    tensor_expr_type mytensor( expr.expression(k), geom );
                    M_t_expr.push_back( mytensor );
                }
            }
        template<typename IM>
        void init( IM const& im )
        {
            for(int i=0; i<M_t_expr.size(); i++)
                M_t_expr[i].init( im );
        }
        FEELPP_DONT_INLINE void updateFun(Geo_t const& geom )
            {
                M_gmc =  fusion::at_key<key_type>( geom ).get();

                int no = 1;
                int ni = M_nsyms;///gmc_type::nDim;

                for(int q = 0; q < M_gmc->nPoints();++q )
                {
                    for ( auto const& comp : M_expr.indexSymbolXYZ() )
                        M_x[comp.second] = M_gmc->xReal( q )[comp.first];
                    for(int i=0; i<M_t_expr_index.size(); i++)
                    {
                        if ( M_t_expr_index[i] != -1 )
                            M_x[M_t_expr_index[i]] = M_t_expr[i].evalq( 0, 0, q );
                    }
                    uint16_type k=0;
                    hana::for_each( M_t_expr2, [&k,&q,this]( auto const& e )
                                    {
                                        uint16_type idx = M_t_expr2_index[k];
                                        if ( idx != invalid_uint16_type_value )
                                            M_x[idx] = e.evalq( 0, 0, q );
                                        ++k;
                                    });
                    M_fun(&ni,M_x.data(),&no,&M_y[q]);
                }
            }
        void update( Geo_t const& geom, Basis_i_t const& fev, Basis_j_t const& feu )
        {
            if ( M_is_zero ) return;

            for(int i=0; i<M_t_expr.size(); i++)
                if ( M_t_expr_index[i] != -1 )
                    M_t_expr[i].update( geom, fev, feu );
            uint16_type k=0;
            hana::for_each( M_t_expr2, [&k,&geom,&fev,feu,this]( auto & e )
                            {
                                if ( M_t_expr2_index[k] != invalid_uint16_type_value )
                                    e.update( geom,fev,feu );
                                ++k;
                            });
            updateFun( geom );

        }
        void update( Geo_t const& geom, Basis_i_t const& fev )
            {
                if ( M_is_zero ) return;

                for(int i=0; i<M_t_expr.size(); i++)
                    if ( M_t_expr_index[i] != -1 )
                        M_t_expr[i].update( geom, fev );
                uint16_type k=0;
                hana::for_each( M_t_expr2, [&k,&geom,&fev,this]( auto & e )
                                {
                                    if ( M_t_expr2_index[k] != invalid_uint16_type_value )
                                        e.update( geom,fev );
                                    ++k;
                                });
                updateFun( geom );
            }
        void update( Geo_t const& geom )
            {
                if ( M_is_zero ) return;

                for(int i=0; i<M_t_expr.size(); i++)
                    if ( M_t_expr_index[i] != -1 )
                        M_t_expr[i].update( geom );
                uint16_type k=0;
                hana::for_each( M_t_expr2, [&k,&geom,this]( auto & e )
                                {
                                    if ( M_t_expr2_index[k] != invalid_uint16_type_value )
                                        e.update( geom );
                                    ++k;
                                });
                updateFun( geom );
            }

        void update( Geo_t const& geom, uint16_type face )
            {
                if ( M_is_zero ) return;

                for(int i=0; i<M_t_expr.size(); i++)
                    if ( M_t_expr_index[i] != -1 )
                        M_t_expr[i].update( geom, face );
                uint16_type k=0;
                hana::for_each( M_t_expr2, [&k,&geom,&face,this]( auto & e )
                                {
                                    if ( M_t_expr2_index[k] != invalid_uint16_type_value )
                                        e.update( geom, face );
                                    ++k;
                                });
                updateFun( geom );
            }

        template<typename ... CTX>
        void updateContext( CTX const& ... ctx )
            {
                if ( M_is_zero ) return;
                boost::fusion::vector<CTX...> ctxvec( ctx... );
                update( boost::fusion::at_c<0>( ctxvec )->gmContext() );
            }

        value_type
        evalij( uint16_type i, uint16_type j ) const
            {
                return 0;
            }


        value_type
        evalijq( uint16_type /*i*/, uint16_type /*j*/, uint16_type c1, uint16_type c2, uint16_type q ) const
            {
                return M_y[q];
            }



        value_type
        evaliq( uint16_type i, uint16_type c1, uint16_type c2, uint16_type q ) const
            {
                return M_y[q];
            }

        value_type
        evalq( uint16_type c1, uint16_type c2, uint16_type q ) const
            {
                return M_y[q];
            }

        this_type const& M_expr;
        GiNaC::FUNCP_CUBA M_fun;
        std::vector<tensor_expr_type> M_t_expr;
        const std::vector<int> M_t_expr_index;
        const bool M_is_zero;
        tuple_tensor_expr2_type M_t_expr2;
        const std::vector<uint16_type> M_t_expr2_index;
        gmc_ptrtype M_gmc;

        int M_nsyms;
        vec_type M_y;
        vec_type M_x;

    };

    value_type
    evaluate( std::map<std::string,value_type> const& mp  )
    {
        this->setParameterValues( mp );
        int no = 1;
        int ni = M_syms.size();//gmc_type::nDim;
        value_type res;
        (*M_cfun)(&ni,M_params.data(),&no,&res);
        return res;
    }

private:
    mutable ginac_expression_type  M_fun;
    std::vector< std::pair<GiNaC::symbol,expression_type> > M_expr;
    boost::shared_ptr<GiNaC::FUNCP_CUBA> M_cfun;
    std::string M_filename;
    std::string M_exprDesc;
    symbols_expression_type M_expr2;
};

///\endcond detail
}} // feel::vf

#endif
