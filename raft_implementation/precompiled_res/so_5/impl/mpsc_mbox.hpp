/*
	SObjectizer 5.
*/

/*!
	\file
	\since
	v.5.4.0

	\brief A multi-producer/single-consumer mbox definition.
*/

#pragma once

#include <so_5/types.hpp>
#include <so_5/exception.hpp>
#include <so_5/spinlocks.hpp>

#include <so_5/mbox.hpp>
#include <so_5/event_queue.hpp>
#include <so_5/message_limit.hpp>

#include <so_5/impl/msg_tracing_helpers.hpp>
#include <so_5/impl/message_limit_internals.hpp>

namespace so_5
{

namespace impl
{

//
// limitless_mpsc_mbox_template
//

/*!
 * \since
 * v.5.4.0
 *
 * \brief A multi-producer/single-consumer mbox definition.
 *
 * \note Since v.5.5.4 is used for implementation of direct mboxes
 * without controling message limits.
 * \note Renamed from limitless_mpsc_mbox_t to limitless_mpsc_mbox_template
 * in v.5.5.9.
 */
template< typename Tracing_Base >
class limitless_mpsc_mbox_template
	:	public abstract_message_box_t
	,	protected Tracing_Base
{
	public:
		template< typename... Tracing_Args >
		limitless_mpsc_mbox_template(
			mbox_id_t id,
			agent_t * single_consumer,
			Tracing_Args &&... tracing_args )
			:	Tracing_Base{ std::forward< Tracing_Args >( tracing_args )... }
			,	m_id{ id }
			,	m_single_consumer{ single_consumer }
			{}

		mbox_id_t
		id() const override
			{
				return m_id;
			}

		void
		subscribe_event_handler(
			const std::type_index & /*msg_type*/,
			const message_limit::control_block_t * /*limit*/,
			agent_t & subscriber ) override
			{
				std::lock_guard< default_rw_spinlock_t > lock{ m_lock };

				if( &subscriber != m_single_consumer )
					SO_5_THROW_EXCEPTION(
							rc_illegal_subscriber_for_mpsc_mbox,
							"the only one consumer can create subscription to mpsc_mbox" );
				++m_subscriptions_count;
			}

		void
		unsubscribe_event_handlers(
			const std::type_index & /*msg_type*/,
			agent_t & subscriber ) override
			{
				std::lock_guard< default_rw_spinlock_t > lock{ m_lock };

				if( &subscriber != m_single_consumer )
					SO_5_THROW_EXCEPTION(
							rc_illegal_subscriber_for_mpsc_mbox,
							"the only one consumer can remove subscription to mpsc_mbox" );
				if( m_subscriptions_count )
					--m_subscriptions_count;
			}

		std::string
		query_name() const override
			{
				std::ostringstream s;
				s << "<mbox:type=MPSC:id="
						<< m_id << ":consumer=" << m_single_consumer
						<< ">";

				return s.str();
			}

		mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_single_consumer;
			}

		void
		do_deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) override
			{
				typename Tracing_Base::deliver_op_tracer tracer{
						*this, // as Tracing_Base
						*this, // as abstract_message_box_t
						"deliver_message",
						msg_type, message, overlimit_reaction_deep };

				this->do_delivery( tracer, [&] {
					tracer.push_to_queue( m_single_consumer );

					agent_t::call_push_event(
							*m_single_consumer,
							message_limit::control_block_t::none(),
							m_id,
							msg_type,
							message );
				} );
			}

		/*!
		 * \attention Will throw an exception because delivery
		 * filter is not applicable to MPSC-mboxes.
		 */
		void
		set_delivery_filter(
			const std::type_index & /*msg_type*/,
			const delivery_filter_t & /*filter*/,
			agent_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION(
						rc_delivery_filter_cannot_be_used_on_mpsc_mbox,
						"set_delivery_filter is called for MPSC-mbox" );
			}

		void
		drop_delivery_filter(
			const std::type_index & /*msg_type*/,
			agent_t & /*subscriber*/ ) noexcept override
			{}

		environment_t &
		environment() const noexcept override
			{
				return m_single_consumer->so_environment();
			}

	protected :
		/*!
		 * \brief ID of this mbox.
		 */
		const mbox_id_t m_id;

		//! The only consumer of this mbox's messages.
		agent_t * m_single_consumer;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Protection of object from modification.
		 */
		default_rw_spinlock_t m_lock;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Number of active subscriptions.
		 *
		 * \note If zero then all attempts to deliver message or
		 * service request will be ignored.
		 */
		std::size_t m_subscriptions_count = 0;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Helper method to do delivery actions under locked object.
		 *
		 * \tparam L lambda with actual delivery actions.
		 */
		template< typename L >
		void
		do_delivery(
			//! Tracer object to log the case of abscense of subscriptions.
			typename Tracing_Base::deliver_op_tracer const & tracer,
			//! Lambda with actual delivery actions.
			L l )
		{
			read_lock_guard_t< default_rw_spinlock_t > lock{ m_lock };

			if( m_subscriptions_count )
				l();
			else
				tracer.no_subscribers();
		}
};

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for limitless_mpsc_mbox without message delivery tracing.
 */
using limitless_mpsc_mbox_without_tracing =
	limitless_mpsc_mbox_template< msg_tracing_helpers::tracing_disabled_base >;

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for limitless_mpsc_mbox with message delivery tracing.
 */
using limitless_mpsc_mbox_with_tracing =
	limitless_mpsc_mbox_template< msg_tracing_helpers::tracing_enabled_base >;

//
// limitful_mpsc_mbox_template
//

/*!
 * \since
 * v.5.5.4
 *
 * \brief A multi-producer/single-consumer mbox with message limit
 * control.
 *
 * \note Renamed from limitful_mpsc_mbox_t to limitful_mpsc_mbox_template
 * in v.5.5.9.
 *
 * \attention Stores a reference to message limits storage. Because of that
 * this reference must remains correct till the end of the mbox's lifetime.
 *
 */
template< typename Tracing_Base >
class limitful_mpsc_mbox_template
	:	public limitless_mpsc_mbox_template< Tracing_Base >
{
		using base_type = limitless_mpsc_mbox_template< Tracing_Base >;
	public:
		template< typename... Tracing_Args >
		limitful_mpsc_mbox_template(
			//! ID of that mbox.
			mbox_id_t id,
			//! The owner of that mbox.
			agent_t * single_consumer,
			//! This reference must remains correct till the end of
			//! the mbox's lifetime.
			const so_5::message_limit::impl::info_storage_t & limits_storage,
			//! Optional arguments for Tracing_Base.
			Tracing_Args &&... tracing_args )
			:	base_type{
					id,
					single_consumer,
					std::forward< Tracing_Args >( tracing_args )... }
			,	m_limits( limits_storage )
			{}

		void
		do_deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) override
			{
				typename Tracing_Base::deliver_op_tracer tracer(
						*this, // as Tracing_Base
						*this, // as abstract_message_box_t
						"deliver_message",
						msg_type, message, overlimit_reaction_deep );

				this->do_delivery( tracer, [&] {
					using namespace so_5::message_limit::impl;

					auto limit = m_limits.find( msg_type );

					try_to_deliver_to_agent(
							this->m_id,
							*(this->m_single_consumer),
							limit,
							msg_type,
							message,
							overlimit_reaction_deep,
							tracer.overlimit_tracer(),
							[&] {
								tracer.push_to_queue( this->m_single_consumer );

								agent_t::call_push_event(
										*(this->m_single_consumer),
										limit,
										this->m_id,
										msg_type,
										message );
							} );
				} );
			}

	private :
		const so_5::message_limit::impl::info_storage_t & m_limits;
};

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for limitful_mpsc_mbox without message delivery tracing.
 */
using limitful_mpsc_mbox_without_tracing =
	limitful_mpsc_mbox_template< msg_tracing_helpers::tracing_disabled_base >;

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for limitful_mpsc_mbox with message delivery tracing.
 */
using limitful_mpsc_mbox_with_tracing =
	limitful_mpsc_mbox_template< msg_tracing_helpers::tracing_enabled_base >;

} /* namespace impl */

} /* namespace so_5 */
