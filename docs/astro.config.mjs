import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
  redirects: {
    '/': '/en/',
  },
  integrations: [
    starlight({
      title: 'Agnostic',
      defaultLocale: 'en',
      locales: {
        en: { label: 'English' },
        ru: { label: 'Русский' },
      },
      social: [
        { icon: 'github', label: 'GitHub', href: 'https://github.com/agnosticlang/agnostic' },
      ],
      sidebar: [
        {
          label: 'Getting Started',
          translations: { ru: 'Начало работы' },
          slug: 'getting-started',
        },
        {
          label: 'Syntax',
          translations: { ru: 'Синтаксис' },
          slug: 'syntax',
        },
        {
          label: 'Types',
          translations: { ru: 'Типы' },
          slug: 'types',
        },
        {
          label: 'Control Flow',
          translations: { ru: 'Управляющие конструкции' },
          slug: 'control-flow',
        },
        {
          label: 'Functions and Closures',
          translations: { ru: 'Функции и замыкания' },
          slug: 'functions',
        },
        {
          label: 'Structs',
          translations: { ru: 'Структуры' },
          slug: 'structs',
        },
        {
          label: 'Comptime',
          translations: { ru: 'Comptime-блоки' },
          slug: 'comptime',
        },
        {
          label: 'Modules',
          translations: { ru: 'Модули' },
          slug: 'modules',
        },
        {
          label: 'Standard Library',
          translations: { ru: 'Стандартная библиотека' },
          slug: 'standard-library',
        },
        {
          label: 'Toolchain',
          translations: { ru: 'Инструментарий' },
          slug: 'toolchain',
        },
      ],
    }),
  ],
});
