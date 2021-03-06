/**
* Copyright 2017 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


#ifndef ISKILL_H
#define ISKILL_H

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "utils/UniqueID.h"
#include "utils/ISerializable.h"
#include "utils/Delegate.h"
#include "utils/ThreadPool.h"
#include "utils/ParamsMap.h"
#include "SelfInstance.h"
#include "SelfLib.h"

//! This base class wraps all available skills in SELF
class SELF_API ISkill : public ISerializable, public boost::enable_shared_from_this<ISkill>
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<ISkill>			SP;
	typedef boost::weak_ptr<ISkill>				WP;

	enum SkillState 
	{
		INACTIVE,			// skill is inactive (default)
		ACTIVE,				// skill is active
		BLOCKED,			// skill is blocked 
		COMPLETED,			// skill completed successfully
		FAILED				// skill has failed
	};
	typedef Delegate<ISkill *>		SkillDelegate;

	//! Construction
	ISkill() : m_bEnabled( true ), m_eState( INACTIVE )
	{
		NewGUID();
	}
	ISkill( const ISkill & a_Copy ) : m_bEnabled( a_Copy.m_bEnabled ), m_SkillName( a_Copy.m_SkillName ), 
		m_eState( INACTIVE )
	{
		NewGUID();
	}
	virtual ~ISkill()
	{}

	//! Accessors
	bool IsEnabled() const
	{
		return m_bEnabled;
	}
	const std::string & GetSkillName() const
	{
		return m_SkillName;
	}
	SkillState GetState() const
	{
		return m_eState;
	}
	ThreadPool * GetThreadPool() const
	{
		return ThreadPool::Instance();
	}

	void SetSkillName(const std::string & a_SkillName)
	{
		m_SkillName = a_SkillName;
	}

	//! ISkill interface
	virtual bool CanUseSkill() = 0;									//! This should return true if this skill can be used, false is returned otherwise.
	virtual void UseSkill( SkillDelegate a_Callback,				//! This begins using this skill, it returns true on success, this should not block.
			const ParamsMap & a_Params ) = 0;						
	virtual bool AbortSkill() = 0;									//! Invoke this function to stop using this skill, the callback passed to UseSkill() will NOT be invoked after this is called.
	virtual ISkill * Clone() = 0;									//! Make a copy of this skill object.

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

protected:
	//! Types
	struct Request
	{
		Request( SkillDelegate a_Callback, const ParamsMap & a_Params )
			: m_Callback( a_Callback ), m_Params( a_Params )
		{}

		SkillDelegate m_Callback;
		ParamsMap m_Params;
	};
	typedef std::list<Request *> RequestList;


	//! Data
	bool				m_bEnabled;
	std::string			m_SkillName;
	SkillState			m_eState;
	RequestList			m_Requests;

	bool HaveRequests() const
	{
		return ActiveRequest() != NULL;
	}

	Request * ActiveRequest() const
	{
		if ( m_Requests.begin() != m_Requests.end() )
			return m_Requests.front();
		return NULL;
	}

	//! Push a request into the queue, returns true if this is the first active request.
	bool PushRequest( SkillDelegate a_Callback, const ParamsMap & a_Params )
	{
		bool bFirstRequest = m_Requests.begin() == m_Requests.end();
		m_Requests.push_back( new Request( a_Callback, a_Params ) );
		return bFirstRequest;
	}

	//! invokes the callback and pops the current active request from the queue, returns true if there are more requests
	bool PopRequest()
	{
		if ( m_Requests.begin() != m_Requests.end() )
		{
			Request * pRequest = m_Requests.front();
			if ( pRequest->m_Callback.IsValid() )
				pRequest->m_Callback( this );

			delete pRequest;
			m_Requests.pop_front();
		}

		return m_Requests.begin() != m_Requests.end();
	}

	void PopAllRequests()
	{
		for( RequestList::iterator iReq = m_Requests.begin(); iReq != m_Requests.end(); ++iReq )
			delete *iReq;
		m_Requests.clear();
	}
};

#endif
