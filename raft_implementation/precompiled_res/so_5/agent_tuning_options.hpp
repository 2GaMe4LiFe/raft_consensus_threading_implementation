/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.3
 *
 * \file
 * \brief A collector for agent tuning options.
 */

#pragma once

#include <so_5/subscription_storage_fwd.hpp>
#include <so_5/message_limit.hpp>
#include <so_5/priority.hpp>

namespace so_5
{

/*
 * NOTE: copy and move constructors and copy operator is implemented
 * because Visual C++ 12.0 (MSVS2013) doesn't generate it by itself.
 */
//
// agent_tuning_options_t
//
/*!
 * \since
 * v.5.5.3
 *
 * \brief A collector for agent tuning options.
 */
class agent_tuning_options_t
	{
	public :
		agent_tuning_options_t()
			{}
		agent_tuning_options_t(
			const agent_tuning_options_t & o )
			:	m_subscription_storage_factory( o.m_subscription_storage_factory )
			,	m_message_limits( o.m_message_limits )
			,	m_priority( o.m_priority )
			{}
		agent_tuning_options_t(
			agent_tuning_options_t && o )
			:	m_subscription_storage_factory(
					std::move( o.m_subscription_storage_factory ) )
			,	m_message_limits( std::move( o.m_message_limits ) )
			,	m_priority( std::move( o.m_priority ) )
			{}

		friend inline void
		swap(
			so_5::agent_tuning_options_t & a,
			so_5::agent_tuning_options_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_subscription_storage_factory,
						b.m_subscription_storage_factory );
				swap( a.m_message_limits, b.m_message_limits );
				swap( a.m_priority, b.m_priority );
			}

		agent_tuning_options_t &
		operator=( agent_tuning_options_t o )
			{
				swap( *this, o );
				return *this;
			}

		//! Set factory for subscription storage creation.
		agent_tuning_options_t &
		subscription_storage_factory(
			subscription_storage_factory_t factory )
			{
				m_subscription_storage_factory = std::move( factory );

				return *this;
			}

		const subscription_storage_factory_t &
		query_subscription_storage_factory() const
			{
				return m_subscription_storage_factory;
			}

		//! Default subscription storage factory.
		static subscription_storage_factory_t
		default_subscription_storage_factory()
			{
				return so_5::default_subscription_storage_factory();
			}

		message_limit::description_container_t
		giveout_message_limits()
			{
				return std::move( m_message_limits );
			}

		template< typename... Args >
		agent_tuning_options_t &
		message_limits( Args &&... args )
			{
				message_limit::accept_indicators(
						m_message_limits,
						std::forward< Args >( args )... );

				return *this;
			}

		//! Set priority for agent.
		/*! \since
		 * v.5.5.8 */

		agent_tuning_options_t &
		priority( so_5::priority_t v )
			{
				m_priority = v;
				return *this;
			}

		//! Get priority value.
		so_5::priority_t
		query_priority() const
			{
				return m_priority;
			}

	private :
		subscription_storage_factory_t m_subscription_storage_factory =
				default_subscription_storage_factory();

		message_limit::description_container_t m_message_limits;

		//! Priority for agent.
		/*! \since
		 * v.5.5.8 */

		so_5::priority_t m_priority = so_5::prio::default_priority;
	};

} /* namespace so_5 */

