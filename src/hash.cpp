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
 *  Copyright (c) 2022 Maxim Filatov <2chemist@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "hash.h"

namespace hashudr
{

resorces_pool pool;
hash_helper helper;

//-----------------------------------------------------------------------------
// attachment_resources 
//

attachment_resources::attachment_resources(
	FB_UDR_STATUS_TYPE* status, FB_UDR_CONTEXT_TYPE* context, const ISC_UINT64 attachment_id) 
		: attachment_id(attachment_id)
{
	snapshot = { status, context, nullptr };
	pull_up_exceptions();
}

attachment_resources::~attachment_resources() noexcept
{
}

const attachment_snapshot* attachment_resources::current_snapshot(FB_UDR_STATUS_TYPE* status, FB_UDR_CONTEXT_TYPE* context)
{
	if (status) snapshot.status = status;
	if (context) snapshot.context = context;
	snapshot.transaction = nullptr;
	return &snapshot;
}

const ITransaction* attachment_resources::current_transaction(ITransaction* transaction)
{
	if (transaction) snapshot.transaction = transaction;
	return !snapshot.transaction ?
				snapshot.context->getTransaction(snapshot.status)
				: snapshot.transaction;
}

const ISC_LONG attachment_resources::exception_number(const char* exception_name) // simple find num
{
	for (auto x : exceptions)
		if (strcmp(x.name, exception_name) == 0) return x.number;
	return 0;
}

const char* attachment_resources::exception_message(const char* exception_name) // simple find msg
{
	for (auto& x : exceptions)
		if (strcmp(x.name, exception_name) == 0) return x.message;
	return nullptr;
}

void attachment_resources::pull_up_exceptions()
{
	AutoRelease<IAttachment> att;
	AutoRelease<ITransaction> tra;
	AutoRelease<IStatement> stmt;
	AutoRelease<IMessageMetadata> meta;
	AutoRelease<IResultSet> curs;

	enum sql_rslt : short { name = 0, number, message };

	const char* sql_stmt = "\
SELECT CAST(TRIM(ex.rdb$exception_name) AS VARCHAR(63)) AS name,\
       ex.rdb$exception_number as number, ex.rdb$message as message\
  FROM rdb$exceptions ex\
  WHERE TRIM(ex.rdb$exception_name) STARTING WITH 'HASH$'";

	try
	{
		att.reset(snapshot.context->getAttachment(snapshot.status));
		tra.reset(snapshot.context->getTransaction(snapshot.status));
		stmt.reset(att->prepare(snapshot.status, tra, 0, sql_stmt, SQL_DIALECT_CURRENT, IStatement::PREPARE_PREFETCH_METADATA));
		meta.reset(stmt->getOutputMetadata(snapshot.status));
		curs.reset(stmt->openCursor(snapshot.status, tra, NULL, NULL, meta, 0));

		AutoArrayDelete<unsigned char> buffer;
		buffer.reset(new unsigned char[meta->getMessageLength(snapshot.status)]);
		attachment_exception exception;
		for (short i = 0; i < EXCEPTION_ARRAY_SIZE; ++i)
		{
			memset(&exception, '\0', sizeof(attachment_exception));
			if (!curs->isEof(snapshot.status) && curs->fetchNext(snapshot.status, buffer) == IStatus::RESULT_OK)
			{
				memcpy( // this SQL_VARYING, see sql_stmt
					exception.name,
					(buffer + 2 + meta->getOffset(snapshot.status, sql_rslt::name)),
					meta->getLength(snapshot.status, sql_rslt::name) - 2);
				exception.number = *(reinterpret_cast<ISC_LONG*>(buffer + meta->getOffset(snapshot.status, sql_rslt::number)));
				memcpy( // SQL_VARYING
					exception.message,
					(buffer + 2 + meta->getOffset(snapshot.status, sql_rslt::message)),
					meta->getLength(snapshot.status, sql_rslt::message) - 2);
			}
			memcpy(&exceptions[i], &exception, sizeof(attachment_exception));
		}
		curs->close(snapshot.status);
		curs.release();
	}
	catch (...)
	{
		throw std::runtime_error("Pulling exceptions crashed.");
	}
}

//-----------------------------------------------------------------------------
// UDR resorces
//

resorces_pool::resorces_pool()
{
}

resorces_pool::~resorces_pool() noexcept
{
	for (resources_it = resources_map.begin(); resources_it != resources_map.end(); ++resources_it)
	{
		delete (attachment_resources*)(resources_it->second);
		resources_map.erase(resources_it);
	}
}

ISC_UINT64 resorces_pool::initialize_attachment(FB_UDR_STATUS_TYPE* status, FB_UDR_CONTEXT_TYPE* context)
{
	ISC_UINT64 att_id = attachment_id(status, context);
	resources_it = resources_map.find(att_id);
	if (resources_it == resources_map.end())
	{
		attachment_resources* att_resources = new attachment_resources(status, context, att_id);
		resources_map.insert(
			std::pair<ISC_UINT64, attachment_resources*>(att_id, att_resources));
	}
	return att_id;
}

attachment_resources* resorces_pool::current_resources(const ISC_UINT64 attachment_id)
{
	attachment_resources* att_resources;
	resources_it = resources_map.find(attachment_id);
	att_resources = (resources_it == resources_map.end() ? nullptr : resources_it->second);
	return att_resources;
}

void resorces_pool::finalize_attachment(const ISC_UINT64 attachment_id)
{
	resources_it = resources_map.find(attachment_id);
	if (resources_it != resources_map.end()) {
		delete (attachment_resources*)(resources_it->second);
		resources_map.erase(resources_it);
	}
}

ISC_UINT64 resorces_pool::attachment_id(FB_UDR_STATUS_TYPE* status, FB_UDR_CONTEXT_TYPE* context)
{
	ISC_UINT64 attachment_id = 0;

	try
	{
		const ISC_UCHAR info[] = { isc_info_attachment_id, isc_info_end };
		ISC_UCHAR result[16];

		(context->getAttachment(status))->
			getInfo(status, sizeof(info), info, sizeof(result), result);

		ISC_UCHAR* p = result;
		if (*p++ == isc_info_attachment_id)
		{
			const ISC_USHORT l =
				static_cast<ISC_USHORT>(isc_vax_integer(reinterpret_cast<const ISC_SCHAR*>(p), 2));
			p += 2;
			attachment_id = isc_portable_integer(p, l);
		}
		else
			throw;
	}
	catch (...)
	{
		throw std::runtime_error("Get info for attachment ID failed.");
	}

	return attachment_id;
}

//-----------------------------------------------------------------------------
// hash_helper
//

void hash_helper::read_blob(attachment_resources* att_resources, ISC_QUAD* in, std::string* out)
{
	const attachment_snapshot* att_snapshot = (att_resources)->current_snapshot();

	AutoRelease<IAttachment> att;
	AutoRelease<IBlob> blob;

	try
	{
		AutoArrayDelete<unsigned char> buffer;
		unsigned read = 0;

		att.reset(att_snapshot->context->getAttachment(att_snapshot->status));
		blob.reset(att->openBlob(
			att_snapshot->status, const_cast<ITransaction*>(att_resources->current_transaction()), in, 0, NULL));
		buffer.reset(new unsigned char[FB_SEGMENT_SIZE]);
		out->erase();
		for (bool eof = false; !eof; )
		{
			switch (blob->getSegment(att_snapshot->status, FB_SEGMENT_SIZE, buffer, &read))
			{
				case IStatus::RESULT_OK:
				case IStatus::RESULT_SEGMENT:
				{
					out->insert(out->end(),
						reinterpret_cast<char*>(static_cast<unsigned char*>(buffer)),
						reinterpret_cast<char*>(static_cast<unsigned char*>(buffer)) + read);
					break;
				}
				default:
				{
					eof = true;
					continue;
				}
			}
		}
		blob->close(att_snapshot->status);
		blob.release(); 
	}
	catch (...)
	{
		throw std::runtime_error("Error reading BLOB to stream.");
	}
}

std::string hash_helper::hash_key(const HASH hash, const std::string* value)
{
	const auto get_key = [](auto hash, const std::string* value) -> std::string
	{
		hash.addData(value->c_str(), value->length());
		return std::move(hash.finalize().toString());
	};

	switch (hash)
	{
		case HASH::Invalid:		throw std::runtime_error("Invalid HASH method.");
		case HASH::Blake1_224:	return std::move(get_key(Chocobo1::Blake1_224(), value));
		case HASH::Blake1_256:	return std::move(get_key(Chocobo1::Blake1_256(), value));
		case HASH::Blake1_384:	return std::move(get_key(Chocobo1::Blake1_384(), value));
		case HASH::Blake1_512:	return std::move(get_key(Chocobo1::Blake1_512(), value));
		case HASH::Blake2:		return std::move(get_key(Chocobo1::Blake2(), value));
		case HASH::Blake2s:		return std::move(get_key(Chocobo1::Blake2s(), value));
		case HASH::Crc_32:		return std::move(get_key(Chocobo1::CRC_32(), value));
		case HASH::Cshake_128:	throw std::runtime_error("Cshake method is not supported.");
		case HASH::Cshake_256:	throw std::runtime_error("Cshake method is not supported.");
		case HASH::Fnv32_1a:	return std::move(get_key(Chocobo1::FNV32_1a(), value));
		case HASH::Fnv64_1a:	return std::move(get_key(Chocobo1::FNV64_1a(), value));
		case HASH::Has160:		return std::move(get_key(Chocobo1::HAS_160(), value));
		case HASH::Md2:			return std::move(get_key(Chocobo1::MD2(), value));
		case HASH::Md4:			return std::move(get_key(Chocobo1::MD4(), value));
		case HASH::Md5:			return std::move(get_key(Chocobo1::MD5(), value));
		case HASH::Ripemd_128:	return std::move(get_key(Chocobo1::RIPEMD_128(), value));
		case HASH::Ripemd_160:	return std::move(get_key(Chocobo1::RIPEMD_160(), value));
		case HASH::Ripemd_256:	return std::move(get_key(Chocobo1::RIPEMD_256(), value));
		case HASH::Ripemd_320:	return std::move(get_key(Chocobo1::RIPEMD_320(), value));
		case HASH::Siphash:
		{
			const unsigned char key[16] = { 0 }; // keep it simple for now
			return std::move(get_key(Chocobo1::SipHash(key), value));
		}
		case HASH::Sha1:		return std::move(get_key(Chocobo1::SHA1(), value));
		case HASH::Sha2_224:	return std::move(get_key(Chocobo1::SHA2_224(), value));
		case HASH::Sha2_256:	return std::move(get_key(Chocobo1::SHA2_256(), value));
		case HASH::Sha2_384:	return std::move(get_key(Chocobo1::SHA2_384(), value));
		case HASH::Sha2_512:	return std::move(get_key(Chocobo1::SHA2_512(), value));
		case HASH::Sha2_512_224:	return std::move(get_key(Chocobo1::SHA2_512_224(), value));
		case HASH::Sha2_512_256:	return std::move(get_key(Chocobo1::SHA2_512_256(), value));
		case HASH::Sha3_224:	return std::move(get_key(Chocobo1::SHA3_224(), value));
		case HASH::Sha3_256:	return std::move(get_key(Chocobo1::SHA3_256(), value));
		case HASH::Sha3_384:	return std::move(get_key(Chocobo1::SHA3_384(), value));
		case HASH::Sha3_512:	return std::move(get_key(Chocobo1::SHA3_512(), value));
		case HASH::Shake_128:	throw std::runtime_error("Shake method is not supported.");
		case HASH::Shake_256:	throw std::runtime_error("Shake method is not supported.");
		case HASH::Sm3:			return std::move(get_key(Chocobo1::SM3(), value));
		case HASH::Tiger1_128:	return std::move(get_key(Chocobo1::Tiger1_128(), value));
		case HASH::Tiger1_160:	return std::move(get_key(Chocobo1::Tiger1_160(), value));
		case HASH::Tiger1_192:	return std::move(get_key(Chocobo1::Tiger1_192(), value));
		case HASH::Tiger2_128:	return std::move(get_key(Chocobo1::Tiger2_128(), value));
		case HASH::Tiger2_160:	return std::move(get_key(Chocobo1::Tiger2_160(), value));
		case HASH::Tiger2_192:	return std::move(get_key(Chocobo1::Tiger2_192(), value));
		case HASH::Tuple_hash_128:	throw std::runtime_error("Tuple_hash method is not supported.");
		case HASH::Tuple_hash_256:	throw std::runtime_error("Tuple_hash method is not supported.");
		case HASH::Whirlpool:	return std::move(get_key(Chocobo1::Whirlpool(), value));

		default:
			throw std::runtime_error("Invalid HASH method.");
	}
}

//-----------------------------------------------------------------------------
// package hash
//

//-----------------------------------------------------------------------------
// create function key (
//   hash_ smallint not null
//   value_ varchar(...) character set none (or blob sub_type binary),
//  ) returns varchar(128) character set none
//  external name 'hash!key'
//  engine udr;
//
FB_UDR_BEGIN_FUNCTION(key)
	
	DECLARE_RESOURCE

	enum in : short {
		hash = 0, value
	};

	AutoArrayDelete<unsigned> in_types;
	AutoArrayDelete<unsigned> in_sub_types;
	AutoArrayDelete<unsigned> in_lengths;
	AutoArrayDelete<unsigned> in_char_sets;
	AutoArrayDelete<unsigned> in_offsets;
	AutoArrayDelete<unsigned> in_null_offsets;
	
	FB_UDR_CONSTRUCTOR
	{
		INITIALIZE_RESORCES

		AutoRelease<IMessageMetadata> in_metadata(metadata->getInputMetadata(status));

		unsigned in_count = in_metadata->getCount(status);

		in_types.reset(new unsigned[in_count]);
		in_sub_types.reset(new unsigned[in_count]);
		in_lengths.reset(new unsigned[in_count]);
		in_char_sets.reset(new unsigned[in_count]);
		in_offsets.reset(new unsigned[in_count]);
		in_null_offsets.reset(new unsigned[in_count]);

		for (unsigned i = 0; i < in_count; ++i)
		{
			in_types[i] = in_metadata->getType(status, i);
			in_sub_types[i] = in_metadata->getSubType(status, i);
			in_lengths[i] = in_metadata->getLength(status, i);
			in_char_sets[i] = in_metadata->getCharSet(status, i);
			in_offsets[i] = in_metadata->getOffset(status, i);
			in_null_offsets[i] = in_metadata->getNullOffset(status, i);
		}
	}

	FB_UDR_DESTRUCTOR
	{
		FINALIZE_RESORCES
	}

	FB_UDR_MESSAGE(
		OutMessage,
		(FB_VARCHAR(128), key)
	);

	FB_UDR_EXECUTE_FUNCTION
	{
		ATTACHMENT_RESORCES
		out->keyNull = FB_TRUE;
		if (!*(reinterpret_cast<ISC_SHORT*>(in + in_null_offsets[in::value])))
		{
			try
			{
				std::string value;
				switch (in_types[in::value])
				{
					case SQL_TEXT: // char
					case SQL_VARYING: // varchar
					{
						if (in_char_sets[in::value] != 0 /* CS_NONE */)
							throw std::runtime_error("CS_NONE character set is allowed.");
						ISC_USHORT length =
							static_cast<ISC_USHORT>(
								(in_types[in::value] == SQL_TEXT ?
									in_lengths[in::value] :	// полный размер переданного CHAR(N) с учетом пробелов 
									*(reinterpret_cast<ISC_USHORT*>(in + in_offsets[in::value]))
									)
								);
						value.assign(
							reinterpret_cast<const char*>(in + (in_types[in::value] == SQL_TEXT ? 0 : sizeof(ISC_USHORT)) + in_offsets[in::value]),
							length
						);
						break;
					}
					case SQL_BLOB: // blob
					{
						if (in_sub_types[in::value] != 0 /* SUB_TYPE BINARY */)
							throw std::runtime_error("SUB_TYPE BINARY is allowed for BLOB.");
						helper.read_blob(att_resources, reinterpret_cast<ISC_QUAD*>(in + in_offsets[in::value]), &value);
						break; 
					}
					default:
						throw std::runtime_error("Allowed [VAR]CHAR(N) or BLOB SQL datatype.");
				}
				std::string key =
					helper.hash_key(
						!*(reinterpret_cast<ISC_SHORT*>(in + in_null_offsets[in::hash])) ?
							static_cast<HASH>(*reinterpret_cast<ISC_SHORT*>(in + in_offsets[in::hash]))
							: HASH::Invalid,
						&value
					);
				out->key.length = static_cast<ISC_SHORT>(key.length());
				memcpy(out->key.str, key.c_str(), out->key.length);
				out->keyNull = FB_FALSE;
			}
			catch (std::runtime_error const& e)
			{
				HASHUDR_THROW(e.what())
			}
		} 
	}

FB_UDR_END_FUNCTION

} // namespace hashudr

FB_UDR_IMPLEMENT_ENTRY_POINT
