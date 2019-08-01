/*
 * Copyright 2010-2019 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the license/LICENSE.txt file.
 */

package com.jetbrains.konan

import com.intellij.lang.Language
import com.intellij.openapi.fileTypes.LanguageFileType
import org.jetbrains.kotlin.idea.KotlinFileType

class KonanLanguage : Language("Kotlin/Native") {
    override fun getAssociatedFileType(): LanguageFileType? = KotlinFileType.INSTANCE

    override fun isCaseSensitive(): Boolean = true

    override fun getDisplayName() = "Kotlin/Native"

    companion object {
        val instance = KonanLanguage()
    }
}