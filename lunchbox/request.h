
/* Copyright (c) 2013-2014, Stefan.Eilemann@epfl.ch
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LUNCHBOX_REQUEST_H
#define LUNCHBOX_REQUEST_H

#include <lunchbox/future.h>

namespace lunchbox
{

/**
 * A Future implementation for a RequestHandler request.
 * @version 1.9.1
 */
template< class T > class Request : public Future< T >
{
    class Impl : public FutureImpl< T >
    {
    public:
        Impl( RequestHandler& handler, const uint32_t req )
            : request( req )
            , _handler( handler )
            , _result( 0 )
            , _done( false )
            , _relinquished( false )
        {}
        virtual ~Impl() {}

        const uint32_t request;

        void relinquish() { _relinquished = true; }
        bool isRelinquished() const { return _relinquished; }

    protected:
        T wait( const uint32_t timeout ) final;
        bool isReady() const final;

    private:
        typedef typename boost::mpl::if_< boost::is_same< T, void >,
                                          void*, T >::type value_t;
        RequestHandler& _handler;
        value_t _result;
        bool _done; //!< waitRequest finished
        bool _relinquished;
    };

public:
    Request( RequestHandler& handler, const uint32_t request )
        : Future< T >( new Impl( handler, request ))
    {}

    /**
     * Destruct and wait for completion of the request, unless relinquished.
     * @version 1.9.1
     */
    virtual ~Request()
    {
        if( !static_cast< const Impl* >( this->impl_.get( ))->isRelinquished( ))
            this->wait();
    }

    /** @return the identifier of the request. @version 1.9.1 */
    uint32_t getID() const
        { return static_cast< const Impl* >( this->impl_.get( ))->request; }

    /**
     * Abandon the request.
     *
     * If called, wait will not be called at destruction and wait() will throw.
     * If the future has already been resolved this function has no effect.
     * @version 1.9.1
     */
    void relinquish()
        { static_cast< Impl* >( this->impl_.get( ))->relinquish(); }
};

}

#include <lunchbox/requestHandler.h>
namespace lunchbox
{
template< class T > inline T Request< T >::Impl::wait(
    const uint32_t timeout )
{
    if( !_done )
    {
        if( _relinquished )
            LBUNREACHABLE;

        if ( !_handler.waitRequest( request, _result, timeout ))
            throw FutureTimeout();
        _done = true;
    }
    return _result;
}

template<> inline void Request< void >::Impl::wait( const uint32_t timeout )
{
    if( !_done )
    {
        if( _relinquished )
            LBUNREACHABLE;

        if ( !_handler.waitRequest( request, _result, timeout ))
            throw FutureTimeout();
        _done = true;
    }
}

template< class T > inline bool Request< T >::Impl::isReady() const
{
    return _done || ( !_relinquished && _handler.isRequestReady( request ));
}

}

#endif //LUNCHBOX_REQUEST_H
