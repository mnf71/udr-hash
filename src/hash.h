 /*
  *  The contents of this file are subject to the Initial
  *  Developer's Public License Version 1.0 (the "License");
  *  you may not use this file except in compliance with the
  *  License. You may obtain a copy of the License at
  *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
  *
  *  Software distributed under the License is distributed AS IS,
  *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
  *  See the License for the specific language governing rights
  *  and limitations under the License.
  *
  *  The Original Code was created by Maxim Filatov.
  *
  *  Copyright (c) 2022 Maxim Filatov <maksim.filatov@moex.com>
  *  and all contributors signed below.
  *
  *  All Rights Reserved.
  *  Contributor(s): ______________________________________.
  */

#ifndef HASH_H
#define HASH_H

#define FB_UDR_STATUS_TYPE ::Firebird::ThrowStatusWrapper
#define FB_UDR_CONTEXT_TYPE ::Firebird::IExternalContext

#include <ibase.h>
#include <UdrCppEngine.h>

#include <stdexcept>
#include <map>

#include <blake1_224.h>
#include <blake1_256.h>
#include <blake1_384.h>
#include <blake1_512.h>
#include <blake2.h>
#include <blake2s.h>
#include <crc_32.h>
#include <cshake.h>
#include <fnv.h>
#include <has_160.h>
#include <md2.h>
#include <md4.h>
#include <md5.h>
#include <ripemd_128.h>
#include <ripemd_160.h>
#include <ripemd_256.h>
#include <ripemd_320.h>
#include <siphash.h>
#include <sha1.h>
#include <sha2_224.h>
#include <sha2_256.h>
#include <sha2_384.h>
#include <sha2_512.h>
#include <sha2_512_224.h>
#include <sha2_512_256.h>
#include <sha3.h>
#include <sm3.h>
#include <tiger.h>
#include <tuple_hash.h>
#include <whirlpool.h> 

using namespace Firebird;

#include <string.h>

namespace hashudr
{

//-----------------------------------------------------------------------------
//

#define	RESULT_BLANK	FB_INTEGER	// domain types
#define	BLANK			-1	// void function emulation

#define FB_SEGMENT_SIZE	32768	// BLOB segment size

//-----------------------------------------------------------------------------
//

#define	HASHUDR_THROW(exception_message)	\
{	\
	if (att_resources == nullptr)	\
	{	\
		ISC_STATUS_ARRAY vector = {	\
			isc_arg_gds,	\
			isc_exception_name, isc_arg_string, reinterpret_cast<ISC_STATUS>(HASHUDR_ERROR),	\
			isc_arg_gds,	\
			isc_random, isc_arg_string, reinterpret_cast<ISC_STATUS>((exception_message)),	\
			isc_arg_end};	\
		status->setErrors(vector);	\
	}	\
	else	\
	{	\
		const ISC_LONG exception_number = att_resources->exception_number(HASHUDR_ERROR);	\
		if (exception_number == 0)	\
		{	\
			ISC_STATUS_ARRAY vector = { \
				isc_arg_gds,	\
				isc_exception_name, isc_arg_string, reinterpret_cast<ISC_STATUS>(HASHUDR_ERROR),	\
				isc_arg_gds,	\
				isc_random, isc_arg_string, reinterpret_cast<ISC_STATUS>((exception_message)),	\
				isc_arg_end };	\
			status->setErrors(vector);	\
		}	\
		else	\
		{	\
			ISC_STATUS_ARRAY vector = { \
				isc_arg_gds,	\
				isc_except, isc_arg_number, exception_number,	\
				isc_arg_gds,	\
				isc_exception_name, isc_arg_string, reinterpret_cast<ISC_STATUS>(HASHUDR_ERROR),	\
				isc_arg_gds,	\
				isc_random, isc_arg_string, reinterpret_cast<ISC_STATUS>((exception_message)),	\
				isc_arg_end };	\
			status->setErrors(vector);	\
		}	\
	}	\
	return; \
} /* HASHUDR_THROW */	

#define	FB_UDR_BEGIN_FUNCTION_RESOURCE(name)	\
	FB_UDR_BEGIN_FUNCTION(name)	\
		const ISC_UINT64 att_id = 0;	\
		attachment_resources* att_resources = nullptr;	\
/* FB_UDR_BEGIN_FUNCTION_RESOURCE */ 

#define	INITIALIZE_RESORCES	\
{	\
	try		\
	{	\
		const_cast<ISC_UINT64&>(att_id) = dispather.initialize_attachment(status, context);	\
	}	\
	catch (std::runtime_error const& e)	\
	{	\
		HASHUDR_THROW(e.what())	\
	}	\
} /* INITIALIZE_RESORCES */ 

#define	ATTACHMENT_RESORCES	\
{	\
	att_resources = dispather.current_resources(att_id);	\
	if (att_resources == nullptr)	\
		HASHUDR_THROW("Attachment resources undefined.")	\
	att_resources->current_snapshot(status, context);	\
	att_resources->current_transaction();	\
} /* ATTACHMENT_RESORCES */

#define	FINALIZE_RESORCES	\
{	\
	dispather.finalize_attachment(att_id);	\
} /* FINALIZE_RESORCES */

//-----------------------------------------------------------------------------
//  ласс обслуживани€ собственого экземпл€ра ресурсов коннекта.
//
// —оздание собственного экземл€ра класса ресурсов необходимо дл€ разграничени€ данных,
// так как SuperServer загружает единственный экземпл€ра бибилиотеки дл€ всех потоков процесса. 
// ’от€ ClassicServer загружает собственные экземпл€р бибилиотеки дл€ всех запущенных процессов,
// избыточность реализации управлени€ ресурсами - один к одному - не €вл€етс€ недостатком.
// 

#define EXCEPTION_ARRAY_SIZE	1	// увеличить при необходимости
#define ERROR_MESSAGE_LENGTH	1024

#define HASHUDR_ERROR			"HASH$HASHUDR_ERROR"

struct attachment_exception
{
	char name[64];
	ISC_LONG number;
	char message[ERROR_MESSAGE_LENGTH];
};

struct attachment_snapshot
{
	FB_UDR_STATUS_TYPE* status;
	FB_UDR_CONTEXT_TYPE* context;
	ITransaction* transaction;
};

class attachment_resources
{
public:	
	attachment_resources(FB_UDR_STATUS_TYPE* status, FB_UDR_CONTEXT_TYPE* context, const ISC_UINT64 attachment_id);
	~attachment_resources() noexcept;

	const ISC_UINT64 current_attachment_id() {	return attachment_id; };
	const attachment_snapshot* current_snapshot(FB_UDR_STATUS_TYPE* status = nullptr, FB_UDR_CONTEXT_TYPE* context = nullptr);
	const ITransaction* current_transaction(ITransaction* transaction = nullptr);

	const ISC_LONG exception_number(const char* name);
	const char* exception_message(const char* name);

private:
	friend class resorces_dispather;

	const ISC_UINT64 attachment_id;
	
	attachment_exception exceptions[EXCEPTION_ARRAY_SIZE] = {
		{HASHUDR_ERROR,		0, ""}
	};
	attachment_snapshot snapshot;

	void pull_up_exceptions();
};

//-----------------------------------------------------------------------------
//  ласс диспетчерезации ресурсов загруженной UDR библиотеки.
//

using resorces_mapping = std::map<ISC_UINT64, attachment_resources*>;

class resorces_dispather
{
public:
	resorces_dispather();
	~resorces_dispather() noexcept;

	ISC_UINT64 initialize_attachment(FB_UDR_STATUS_TYPE* status, FB_UDR_CONTEXT_TYPE* context);
	attachment_resources* current_resources(const ISC_UINT64 attachment_id);
	void finalize_attachment(const ISC_UINT64 attachment_id);

private:
	resorces_mapping resources_map;
	resorces_mapping::iterator resources_it;

	ISC_UINT64 attachment_id(FB_UDR_STATUS_TYPE* status, FB_UDR_CONTEXT_TYPE* context);
};

extern resorces_dispather dispather;

//-----------------------------------------------------------------------------
//  ласс вспомогательных функций библиотеки
//

enum class HASH : short
{
	Invalid = 0,
	Blake1_224, Blake1_256, Blake1_384, Blake1_512,
	Blake2, Blake2s,
	Crc_32,
	Cshake_128, Cshake_256, /* will in future */
	Fnv32_1a, Fnv64_1a,
	Has160,
	Md2, Md4, Md5,
	Ripemd_128, Ripemd_160, Ripemd_256, Ripemd_320,
	Siphash,
	Sha1,
	Sha2_224, Sha2_256, Sha2_384, Sha2_512,
	Sha2_512_224, Sha2_512_256,
	Sha3_224, Sha3_256, Sha3_384, Sha3_512,
	Shake_128, Shake_256, /* will in future */
	Sm3,
	Tiger1_128, Tiger1_160, Tiger1_192,
	Tiger2_128, Tiger2_160, Tiger2_192,
	Tuple_hash_128, Tuple_hash_256, /* will in future */
	Whirlpool
};

class hash_helper
{
public:
	void read_blob(attachment_resources* att_resources, ISC_QUAD* in, std::string* out);
	std::string hash_key(const HASH hash, const std::string* value);
};

extern hash_helper helper;

} // namespace hashudr

/*
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2015 Adriano dos Santos Fernandes <adrianosf@gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include <assert.h>
#include <stdio.h>

namespace
{
	template <typename T>
	class AutoReleaseClear
	{
	public:
		static void clear(T* ptr)
		{
			if (ptr)
				ptr->release();
		}
	};

	template <typename T>
	class AutoDisposeClear
	{
	public:
		static void clear(T* ptr)
		{
			if (ptr)
				ptr->dispose();
		}
	};

	template <typename T>
	class AutoDeleteClear
	{
	public:
		static void clear(T* ptr)
		{
			delete ptr;
		}
	};

	template <typename T>
	class AutoArrayDeleteClear
	{
	public:
		static void clear(T* ptr)
		{
			delete[] ptr;
		}
	};

	template <typename T, typename Clear>
	class AutoImpl
	{
	public:
		AutoImpl<T, Clear>(T* aPtr = NULL)
			: ptr(aPtr)
		{
		}

		~AutoImpl()
		{
			Clear::clear(ptr);
		}

		AutoImpl<T, Clear>& operator =(T* aPtr)
		{
			Clear::clear(ptr);
			ptr = aPtr;
			return *this;
		}

		operator T* ()
		{
			return ptr;
		}

		operator const T* () const
		{
			return ptr;
		}

		bool operator !() const
		{
			return !ptr;
		}

		bool hasData() const
		{
			return ptr != NULL;
		}

		T* operator ->()
		{
			return ptr;
		}

		T* release()
		{
			T* tmp = ptr;
			ptr = NULL;
			return tmp;
		}

		void reset(T* aPtr = NULL)
		{
			if (aPtr != ptr)
			{
				Clear::clear(ptr);
				ptr = aPtr;
			}
		}

	private:
		AutoImpl<T, Clear>(AutoImpl<T, Clear>&); // not implemented
		void operator =(AutoImpl<T, Clear>&);

	private:
		T* ptr;
	};

	template <typename T> class AutoDispose : public AutoImpl<T, AutoDisposeClear<T> >
	{
	public:
		AutoDispose(T* ptr = NULL)
			: AutoImpl<T, AutoDisposeClear<T> >(ptr)
		{
		}
	};

	template <typename T> class AutoRelease : public AutoImpl<T, AutoReleaseClear<T> >
	{
	public:
		AutoRelease(T* ptr = NULL)
			: AutoImpl<T, AutoReleaseClear<T> >(ptr)
		{
		}
	};

	template <typename T> class AutoDelete : public AutoImpl<T, AutoDeleteClear<T> >
	{
	public:
		AutoDelete(T* ptr = NULL)
			: AutoImpl<T, AutoDeleteClear<T> >(ptr)
		{
		}
	};

	template <typename T> class AutoArrayDelete : public AutoImpl<T, AutoArrayDeleteClear<T> >
	{
	public:
		AutoArrayDelete(T* ptr = NULL)
			: AutoImpl<T, AutoArrayDeleteClear<T> >(ptr)
		{
		}
	};
}

#endif	/* HASH_H */     