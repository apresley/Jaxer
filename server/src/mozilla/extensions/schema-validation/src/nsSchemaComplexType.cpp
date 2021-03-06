/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Vidur Apparao <vidur@netscape.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsSchemaPrivate.h"

////////////////////////////////////////////////////////////
//
// nsSchemaComplexType implementation
//
////////////////////////////////////////////////////////////
nsSchemaComplexType::nsSchemaComplexType(nsSchema* aSchema,
                                         const nsAString& aName,
                                         PRBool aAbstract)
  : nsSchemaComponentBase(aSchema), mName(aName), mAbstract(aAbstract),
    mContentModel(CONTENT_MODEL_ELEMENT_ONLY),
    mDerivation(DERIVATION_SELF_CONTAINED)
{
}

nsSchemaComplexType::~nsSchemaComplexType()
{
}

NS_IMPL_ISUPPORTS3(nsSchemaComplexType,
                      nsISVSchemaComponent,
                      nsISVSchemaType,
                      nsISVSchemaComplexType)


nsresult
nsSchemaComplexType::ProcessExtension(nsISVSchemaErrorHandler* aErrorHandler)
{
  nsresult rv = NS_OK;

  nsAutoString baseStr;
  mBaseType->GetName(baseStr);

  nsCOMPtr<nsISVSchemaComplexType> complexBaseType(do_QueryInterface(mBaseType));

  nsCOMPtr<nsISVSchemaModelGroup> sequence;
  nsSchemaModelGroup* sequenceInst = nsnull;
  if (complexBaseType) {
    // XXX Should really be cloning
    nsCOMPtr<nsISVSchemaModelGroup> baseGroup;
    rv = complexBaseType->GetModelGroup(getter_AddRefs(baseGroup));
    if (NS_FAILED(rv)) {
      nsAutoString errorMsg;
      errorMsg.AppendLiteral("Failure processing schema, ");
      errorMsg.AppendLiteral("extension for type \"");
      errorMsg.Append(baseStr);
      errorMsg.AppendLiteral("\" does not contains any model group");
      errorMsg.AppendLiteral("such as <all>, <choice>, <sequence>, or <group>");

      NS_SCHEMALOADER_FIRE_ERROR(rv, errorMsg);

      return rv;
    }
          
    if (baseGroup) {
      // Create a new model group that's going to be the a sequence
      // of the base model group and the content below
      sequenceInst = new nsSchemaModelGroup(mSchema, EmptyString());
      if (!sequenceInst) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      sequence = sequenceInst;

      PRUint16 compositor;
      baseGroup->GetCompositor(&compositor);

      PRUint32 minOccurs, maxOccurs;
      baseGroup->GetMinOccurs(&minOccurs);
      baseGroup->GetMaxOccurs(&maxOccurs);

      // If the base group also a sequence, we can collapse the
      // two sequences.
      if ((compositor == nsISVSchemaModelGroup::COMPOSITOR_SEQUENCE) &&
          (minOccurs == 1) && (maxOccurs == 1)) {
        PRUint32 pIndex, pCount;
        baseGroup->GetParticleCount(&pCount);
        for (pIndex = 0; pIndex < pCount; pIndex++) {
          nsCOMPtr<nsISVSchemaParticle> particle;
                
          rv = baseGroup->GetParticle(pIndex, getter_AddRefs(particle));
          if (NS_FAILED(rv)) {
            nsAutoString errorMsg;
            errorMsg.AppendLiteral("Failure processing schema, failure ");
            errorMsg.AppendLiteral("processing model group for extension ");
            errorMsg.AppendLiteral("of type \"");
            errorMsg.Append(baseStr);
            errorMsg.AppendLiteral("\"");

            NS_SCHEMALOADER_FIRE_ERROR(rv, errorMsg);

            return rv;
          }
                
          rv = sequenceInst->AddParticle(particle);
          if (NS_FAILED(rv)) {
            nsAutoString errorMsg;
            errorMsg.AppendLiteral("Failure processing schema, failure ");
            errorMsg.AppendLiteral("processing model group for extension ");
            errorMsg.AppendLiteral("of type \"");
            errorMsg.Append(baseStr);
            errorMsg.AppendLiteral("\"");

            NS_SCHEMALOADER_FIRE_ERROR(rv, errorMsg);

            return rv;
          }
        }
      }
      else {
        sequenceInst->AddParticle(baseGroup);
      }
            
      // Need to append original contents.
      if (mModelGroup) {
        PRUint32 pIndex, pCount;
        mModelGroup->GetParticleCount(&pCount);
        for (pIndex = 0; pIndex < pCount; pIndex++) {
          nsCOMPtr<nsISVSchemaParticle> particle;
          rv = mModelGroup->GetParticle(pIndex, getter_AddRefs(particle));
          rv = sequenceInst->AddParticle(particle);
        }
      }
      this->SetModelGroup(sequence);
    }
  }
        
  if (complexBaseType) {
    // Inherit content model from base if currently empty
    if (mContentModel == nsISVSchemaComplexType::CONTENT_MODEL_EMPTY) {
      PRUint16 baseContentModel;
      complexBaseType->GetContentModel(&baseContentModel);
      mContentModel = baseContentModel;
    }

    // Copy over the attributes from the base type
    // XXX Should really be cloning
    PRUint32 attrIndex, attrCount;
    complexBaseType->GetAttributeCount(&attrCount);

    for (attrIndex = 0; attrIndex < attrCount; attrIndex++) {
      nsCOMPtr<nsISVSchemaAttributeComponent> attribute;

      rv = complexBaseType->GetAttributeByIndex(attrIndex,
                                                getter_AddRefs(attribute));
      if (NS_FAILED(rv)) {
        nsAutoString errorMsg;
        errorMsg.AppendLiteral("Failure processing schema, cannot clone ");
        errorMsg.AppendLiteral("attributes from base type \"");
        errorMsg.Append(baseStr);
        errorMsg.AppendLiteral("\"");

        NS_SCHEMALOADER_FIRE_ERROR(rv, errorMsg);

        return rv;
      }

      rv = this->AddAttribute(attribute);
      if (NS_FAILED(rv)) {
        nsAutoString errorMsg;
        errorMsg.AppendLiteral("Failure processing schema, cannot clone ");
        errorMsg.AppendLiteral("attributes from base type \"");
        errorMsg.Append(baseStr);
        errorMsg.AppendLiteral("\"");

        NS_SCHEMALOADER_FIRE_ERROR(rv, errorMsg);

        return rv;
      }
    }
  }
  return NS_OK;
}


/* void resolve (in nsISVSchemaErrorHandler* aErrorHandler); */
NS_IMETHODIMP
nsSchemaComplexType::Resolve(nsISVSchemaErrorHandler* aErrorHandler)
{
  if (mIsResolved) {
    return NS_OK;
  }

  mIsResolved = PR_TRUE;
  nsresult rv;
  PRUint32 i, count;

  count = mAttributes.Count();
  for (i = 0; i < count; ++i) {
    rv = mAttributes.ObjectAt(i)->Resolve(aErrorHandler);
    if (NS_FAILED(rv)) {
      nsAutoString attrName;
      nsresult rv1 = mAttributes.ObjectAt(i)->GetName(attrName);
      NS_ENSURE_SUCCESS(rv1, rv1);

      nsAutoString errorMsg;
      errorMsg.AppendLiteral("Failure resolving schema complex type, ");
      errorMsg.AppendLiteral("cannot resolve attribute \"");
      errorMsg.Append(attrName);
      errorMsg.AppendLiteral("\"");
      
      NS_SCHEMALOADER_FIRE_ERROR(rv, errorMsg);

      return rv;
    }
  }

  if (!mSchema) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsISVSchemaType> type;
  if (mBaseType) {
    rv = mSchema->ResolveTypePlaceholder(aErrorHandler, mBaseType, getter_AddRefs(type));
    if (NS_FAILED(rv)) {
      return NS_ERROR_FAILURE;
    }
    // Only process unresolved base types, as resolved base types are
    // processed when they are encountered during main parsing phase.
    if (mBaseType != type) {
      mBaseType = type;
      rv = mBaseType->Resolve(aErrorHandler);
      if (NS_FAILED(rv)) {
        nsAutoString baseStr;
        nsresult rv1 = type->GetName(baseStr);
        NS_ENSURE_SUCCESS(rv1, rv1);

        nsAutoString errorMsg;
        errorMsg.AppendLiteral("Failure resolving schema complex type, ");
        errorMsg.AppendLiteral("cannot resolve base type \"");
        errorMsg.Append(baseStr);
        errorMsg.AppendLiteral("\"");

        NS_SCHEMALOADER_FIRE_ERROR(rv, errorMsg);
        return NS_ERROR_FAILURE;
      }

      // Only extensions need to be applied
      if (mDerivation == nsISVSchemaComplexType::DERIVATION_EXTENSION_COMPLEX) {
        rv = ProcessExtension(aErrorHandler);
        if (NS_FAILED(rv)) {
          return NS_ERROR_FAILURE;
        }
      }
    }
  }
    
  if (mSimpleBaseType) {
    rv = mSchema->ResolveTypePlaceholder(aErrorHandler, mSimpleBaseType,
                                         getter_AddRefs(type));
    if (NS_FAILED(rv)) {
      return NS_ERROR_FAILURE;
    }

    mSimpleBaseType = do_QueryInterface(type);

    // mSimpleBaseType could become a complex type under certain conditions
    // (simple content that restricts a complex type, which itself is a
    // simple content).  So if we can't QI to a simple type, get the simple
    // base type if it is a complex type.
    if (!mSimpleBaseType) {
      nsCOMPtr<nsISVSchemaComplexType> complexType = do_QueryInterface(type);
      if (complexType) {
        complexType->GetSimpleBaseType(getter_AddRefs(mSimpleBaseType));
      }
    }

    if (!mSimpleBaseType) {
      return NS_ERROR_FAILURE;
    }
    rv = mSimpleBaseType->Resolve(aErrorHandler);
    if (NS_FAILED(rv)) {
      return NS_ERROR_FAILURE;
    }
  }

  if (mModelGroup) {
    rv = mModelGroup->Resolve(aErrorHandler);
    if (NS_FAILED(rv)) {
      nsAutoString modelName;
      nsresult rv1 = mModelGroup->GetName(modelName);
      NS_ENSURE_SUCCESS(rv1, rv1);

      nsAutoString errorMsg;
      errorMsg.AppendLiteral("Failure resolving schema complex type, ");
      errorMsg.AppendLiteral("cannot resolve model group \"");
      errorMsg.Append(modelName);
      errorMsg.AppendLiteral("\"");

      NS_SCHEMALOADER_FIRE_ERROR(rv, errorMsg);

      return NS_ERROR_FAILURE;
    }
  }

  if (mArrayInfo) {
    nsCOMPtr<nsISVSchemaType> placeHolder;
    mArrayInfo->GetType(getter_AddRefs(placeHolder));
    if (placeHolder) {
      PRUint16 schemaType;
      placeHolder->GetSchemaType(&schemaType);
      if (schemaType == nsISVSchemaType::SCHEMA_TYPE_PLACEHOLDER) {
        rv = mSchema->ResolveTypePlaceholder(aErrorHandler, placeHolder, getter_AddRefs(type));
        if (NS_FAILED(rv)) {
          return NS_ERROR_FAILURE;
        }
        rv = type->Resolve(aErrorHandler);
        if (NS_FAILED(rv)) {
          return NS_ERROR_FAILURE;
        }
        SetArrayInfo(type, mArrayInfo->GetDimension());
      }
      else {
         rv = placeHolder->Resolve(aErrorHandler);
        if (NS_FAILED(rv)) {
          return NS_ERROR_FAILURE;
        }
      }
    }
  }

  return NS_OK;
}

/* void clear (); */
NS_IMETHODIMP
nsSchemaComplexType::Clear()
{
  if (mIsCleared) {
    return NS_OK;
  }

  mIsCleared = PR_TRUE;
  if (mBaseType) {
    mBaseType->Clear();
    mBaseType = nsnull;
  }
  if (mSimpleBaseType) {
    mSimpleBaseType->Clear();
    mSimpleBaseType = nsnull;
  }
  if (mModelGroup) {
    mModelGroup->Clear();
    mModelGroup = nsnull;
  }

  PRUint32 i, count;
  count = mAttributes.Count();
  for (i = 0; i < count; ++i) {
    mAttributes.ObjectAt(i)->Clear();
  }
  mAttributes.Clear();
  mAttributesHash.Clear();

  return NS_OK;
}

/* readonly attribute wstring name; */
NS_IMETHODIMP
nsSchemaComplexType::GetName(nsAString& aName)
{
  aName.Assign(mName);
  
  return NS_OK;
}

/* readonly attribute unsigned short schemaType; */
NS_IMETHODIMP
nsSchemaComplexType::GetSchemaType(PRUint16 *aSchemaType)
{
  NS_ENSURE_ARG_POINTER(aSchemaType);

  *aSchemaType = nsISVSchemaType::SCHEMA_TYPE_COMPLEX;

  return NS_OK;
}

/* readonly attribute unsigned short contentModel; */
NS_IMETHODIMP
nsSchemaComplexType::GetContentModel(PRUint16 *aContentModel)
{
  NS_ENSURE_ARG_POINTER(aContentModel);
  
  *aContentModel = mContentModel;
  
  return NS_OK;
}

/* readonly attribute unsigned short derivation; */
NS_IMETHODIMP
nsSchemaComplexType::GetDerivation(PRUint16 *aDerivation)
{
  NS_ENSURE_ARG_POINTER(aDerivation);

  *aDerivation = mDerivation;

  return NS_OK;
}

/* readonly attribute nsISVSchemaType baseType; */
NS_IMETHODIMP
nsSchemaComplexType::GetBaseType(nsISVSchemaType * *aBaseType)
{
  NS_ENSURE_ARG_POINTER(aBaseType);

  NS_IF_ADDREF(*aBaseType = mBaseType);

  return NS_OK;
}

/* readonly attribute nsISVSchemaSimpleType simplBaseType; */
NS_IMETHODIMP
nsSchemaComplexType::GetSimpleBaseType(nsISVSchemaSimpleType * *aSimpleBaseType)
{
  NS_ENSURE_ARG_POINTER(aSimpleBaseType);

  NS_IF_ADDREF(*aSimpleBaseType = mSimpleBaseType);

  return NS_OK;
}

/* readonly attribute nsISVSchemaModelGroup modelGroup; */
NS_IMETHODIMP
nsSchemaComplexType::GetModelGroup(nsISVSchemaModelGroup * *aModelGroup)
{
  NS_ENSURE_ARG_POINTER(aModelGroup);

  NS_IF_ADDREF(*aModelGroup = mModelGroup);

  return NS_OK;
}

/* readonly attribute PRUint32 attributeCount; */
NS_IMETHODIMP
nsSchemaComplexType::GetAttributeCount(PRUint32 *aAttributeCount)
{
  NS_ENSURE_ARG_POINTER(aAttributeCount);

  *aAttributeCount = mAttributes.Count();

  return NS_OK;
}

/* nsISVSchemaAttributeComponent getAttributeByIndex (in PRUint32 index); */
NS_IMETHODIMP
nsSchemaComplexType::GetAttributeByIndex(PRUint32 aIndex,
                                         nsISVSchemaAttributeComponent** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  if (aIndex >= (PRUint32)mAttributes.Count()) {
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*aResult = mAttributes.ObjectAt(aIndex));

  return NS_OK;
}

/* nsISVSchemaAttributeComponent getAttributeByName (in AString name); */
NS_IMETHODIMP
nsSchemaComplexType::GetAttributeByName(const nsAString& aName,
                                        nsISVSchemaAttributeComponent** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  mAttributesHash.Get(aName, aResult);

  return NS_OK;
}

/* readonly attribute boolean abstract; */
NS_IMETHODIMP
nsSchemaComplexType::GetAbstract(PRBool *aAbstract)
{
  NS_ENSURE_ARG_POINTER(aAbstract);

  *aAbstract = mAbstract;
  
  return NS_OK;
}

/* readonly attribute boolean isArray; */
NS_IMETHODIMP
nsSchemaComplexType::GetIsArray(PRBool* aIsArray)
{
  NS_ENSURE_ARG_POINTER(aIsArray);

  nsCOMPtr<nsISVSchemaComplexType> complexBase = do_QueryInterface(mBaseType);
  if (complexBase) {
    return complexBase->GetIsArray(aIsArray);
  }

  *aIsArray = PR_FALSE;

  return NS_OK;
}

/* readonly attribute nsISVSchemaType arrayType; */
NS_IMETHODIMP
nsSchemaComplexType::GetArrayType(nsISVSchemaType** aArrayType)
{
  NS_ENSURE_ARG_POINTER(aArrayType);

  *aArrayType = nsnull;
  if (mArrayInfo) {
    mArrayInfo->GetType(aArrayType);
  }
  else {
    nsCOMPtr<nsISVSchemaComplexType> complexBase = do_QueryInterface(mBaseType);
    if (complexBase) {
      return complexBase->GetArrayType(aArrayType);
    }
  }

  return NS_OK;
}

/* readonly attribute PRUint32 arrayDimension; */
NS_IMETHODIMP
nsSchemaComplexType::GetArrayDimension(PRUint32* aDimension)
{
  NS_ENSURE_ARG_POINTER(aDimension);

  *aDimension = 0;
  if (mArrayInfo) {
    *aDimension = mArrayInfo->GetDimension();
  }
  else {
    nsCOMPtr<nsISVSchemaComplexType> complexBase = do_QueryInterface(mBaseType);
    if (complexBase) {
      return complexBase->GetArrayDimension(aDimension);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSchemaComplexType::SetContentModel(PRUint16 aContentModel)
{
  mContentModel = aContentModel;

  return NS_OK;
}

NS_IMETHODIMP
nsSchemaComplexType::SetDerivation(PRUint16 aDerivation,
                                   nsISVSchemaType* aBaseType)
{
  mDerivation = aDerivation;
  mBaseType = aBaseType;

  return NS_OK;
}

NS_IMETHODIMP
nsSchemaComplexType::SetSimpleBaseType(nsISVSchemaSimpleType* aSimpleBaseType)
{
  mSimpleBaseType = aSimpleBaseType;

  return NS_OK;
}

NS_IMETHODIMP
nsSchemaComplexType::SetModelGroup(nsISVSchemaModelGroup* aModelGroup)
{
  mModelGroup = aModelGroup;

  return NS_OK;
}

NS_IMETHODIMP
nsSchemaComplexType::AddAttribute(nsISVSchemaAttributeComponent* aAttribute)
{
  NS_ENSURE_ARG_POINTER(aAttribute);

  nsAutoString name;
  aAttribute->GetName(name);

  mAttributes.AppendObject(aAttribute);
  mAttributesHash.Put(name, aAttribute);

  return NS_OK;
}

NS_IMETHODIMP
nsSchemaComplexType::SetArrayInfo(nsISVSchemaType* aType, PRUint32 aDimension)
{
  mArrayInfo = new nsComplexTypeArrayInfo(aType, aDimension);

  return mArrayInfo ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
