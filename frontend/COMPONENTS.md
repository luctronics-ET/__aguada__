# AGUADA Frontend Components & Styling Guide

## 📋 Overview

This document describes the unified UI components and styling system used across all AGUADA frontend pages.

**System Status**: ✅ All 12 pages using shared layout (header/nav/footer) and CSS components

---

## 🏗️ Shared Layout Architecture

### layout.js (Injected in All Pages)

The `assets/layout.js` script automatically renders and updates:

1. **Admin Header** - Title + Status indicator
2. **Navigation Bar** - Links to all pages with active highlighting
3. **Footer** - Copyright + Last update timestamp (live)

**Active Page Detection**: Automatic based on filename (e.g., `mapa.html` → highlights "Mapa" in nav)

**Usage**:

```html
<script src="assets/layout.js"></script>
<!-- Must be first script tag -->
```

### Pages Currently Using Shared Layout

```
✅ index.html           (Dashboard)
✅ mapa.html            (Map)
✅ painel.html          (Visual Panel)
✅ dados.html           (Data)
✅ consumo.html         (Consumption)
✅ abastecimento.html   (Supply)
✅ manutencao.html      (Maintenance)
✅ history.html         (History)
✅ alerts.html          (Alerts)
✅ config.html          (Settings)
✅ system.html          (System Status)
✅ documentacao.html    (Documentation)
```

**Total**: 12/12 pages using layout.js ✓

---

## 🎨 Shared CSS Components (style.css)

### Color Variables

```css
--primary-blue:     #0066cc
--secondary-blue:   #0052a3
--accent-purple:    #8B7FD9
--success-green:    #10b981
--warning-orange:   #f59e0b
--danger-red:       #ef4444
--text-dark:        #1f2937
--text-muted:       #6b7280
--background-light: #f3f4f6
--border-color:     #e5e7eb
```

### Global Styles (745 lines, 60 CSS classes)

#### 1. **Layout Components**

| Class               | Purpose                                 |
| ------------------- | --------------------------------------- |
| `.admin-header`     | Top navigation container                |
| `.admin-header-top` | Header title + status row               |
| `.admin-nav`        | Navigation bar with active highlighting |
| `.main-container`   | Content wrapper with padding            |
| `.dashboard-grid`   | 3-column responsive grid                |
| `.app-footer`       | Bottom footer bar                       |

#### 2. **Card Components**

| Class          | Purpose                         |
| -------------- | ------------------------------- |
| `.card`        | Whitebox with shadow (standard) |
| `.card-header` | Card title row                  |
| `.card-body`   | Card content area               |
| `.card-footer` | Card bottom row                 |

#### 3. **Table Components**

| Class                   | Purpose                     |
| ----------------------- | --------------------------- |
| `.table-wrapper`        | Container for tables        |
| `.table`                | Standard data table styling |
| `.table thead`          | Header row styling          |
| `.table tbody tr:hover` | Row highlight on hover      |

#### 4. **Filter & Search**

| Class           | Purpose                    |
| --------------- | -------------------------- |
| `.filter-bar`   | Horizontal filter controls |
| `.filter-group` | Filter input group         |
| `.search-box`   | Text input field           |
| `.chip`         | Removable tag/filter chip  |
| `.chip.active`  | Selected chip styling      |

#### 5. **Data Display**

| Class           | Purpose                  |
| --------------- | ------------------------ |
| `.data-row`     | Horizontal data layout   |
| `.data-item`    | Single data field        |
| `.data-label`   | Field label              |
| `.data-value`   | Field value (bold)       |
| `.stat`         | Stat box (value + label) |
| `.stats-footer` | Footer with 4 stats      |

#### 6. **Status Indicators**

| Class               | Purpose                                |
| ------------------- | -------------------------------------- |
| `.status-indicator` | Online/Offline status                  |
| `.status-dot`       | Blinking green dot                     |
| `.status-badge`     | Colored badge (online/offline/warning) |
| `.status-online`    | Green background                       |
| `.status-offline`   | Gray background                        |

#### 7. **Forms & Inputs**

| Class             | Purpose               |
| ----------------- | --------------------- |
| `.form-group`     | Input + label wrapper |
| `.form-input`     | Text input styling    |
| `.form-select`    | Dropdown styling      |
| `.button`         | Standard button       |
| `.button.primary` | Primary blue button   |
| `.button.danger`  | Red danger button     |

#### 8. **Loading & Skeleton**

| Class                           | Purpose                |
| ------------------------------- | ---------------------- |
| `.skeleton`                     | Placeholder animation  |
| `.skeleton-text`                | Text skeleton (1 line) |
| `.skeleton-avatar`              | Avatar skeleton        |
| `.loading-states` (linked file) | All loading animations |

#### 9. **Alerts & Badges**

| Class            | Purpose               |
| ---------------- | --------------------- |
| `.alert`         | Alert box container   |
| `.alert.info`    | Blue info alert       |
| `.alert.warning` | Orange warning alert  |
| `.alert.danger`  | Red danger alert      |
| `.badge`         | Small label tag       |
| `.badge.primary` | Primary colored badge |

#### 10. **Responsive Grid**

| Class             | Purpose                |
| ----------------- | ---------------------- |
| `.dashboard-grid` | 3-column (auto-shrink) |
| `.grid-2`         | 2-column grid          |
| `.grid-4`         | 4-column grid          |

---

## 🎯 CSS Classes Used Across Pages

### index.html (Dashboard)

```
.admin-header, .admin-nav, .dashboard-grid, .card, .stat, .stats-footer
.data-row, .data-item, .status-indicator
```

### dados.html (Data View)

```
.table-wrapper, .table, .filter-bar, .chip
.search-box, .data-row, .status-badge
```

### painel.html (Visual Panel)

```
.dashboard-grid, .card, .status-indicator
.data-value, .progress-bar, .chart-container
```

### alerts.html

```
.filter-bar, .chip, .table, .status-badge
.alert, .badge.danger, .stats-footer
```

### system.html

```
.card-header, .data-row, .status-badge, .stats-footer
.status-online, .progress-fill
```

---

## 🔧 Adding New Pages

To add a new page with shared styling:

1. **Load layout.js first**:

   ```html
   <head>
     <link rel="stylesheet" href="assets/style.css" />
   </head>
   <body>
     <!-- Your content here -->

     <script src="assets/layout.js"></script>
     <!-- FIRST script -->
     <script src="assets/api-service.js"></script>
     <script src="assets/app.js"></script>
   </body>
   ```

2. **Add your page to NAV_ITEMS in layout.js**:

   ```javascript
   const NAV_ITEMS = [
     // ... existing items ...
     { href: "mypage.html", label: "My Page" },
   ];
   ```

3. **Use standard CSS classes**:
   ```html
   <div class="card">
     <div class="card-header">Title</div>
     <div class="card-body">
       <div class="data-row">
         <div class="data-item">
           <div class="data-label">Label</div>
           <div class="data-value">Value</div>
         </div>
       </div>
     </div>
   </div>
   ```

---

## 📐 Responsive Breakpoints

```css
/* Desktop (default) */
.dashboard-grid {
  grid-template-columns: repeat(3, 1fr);
}

/* Tablet (768px - 1024px) */
@media (max-width: 1024px) {
  .dashboard-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}

/* Mobile (< 768px) */
@media (max-width: 768px) {
  .dashboard-grid {
    grid-template-columns: 1fr;
  }
}
```

---

## 🎬 Animation Classes

### Loading Skeleton (from loading-states.css)

```css
.skeleton {
  animation: loading 1.5s infinite;
}

@keyframes loading {
  0%,
  100% {
    opacity: 0.5;
  }
  50% {
    opacity: 1;
  }
}
```

### Status Indicator (Blinking)

```css
.status-dot {
  animation: pulse 2s infinite;
}

@keyframes pulse {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}
```

---

## 📝 Example Page Structure

```html
<!DOCTYPE html>
<html lang="pt-BR">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>My Page - AGUADA</title>
    <link rel="stylesheet" href="assets/style.css" />
  </head>
  <body>
    <!-- Header auto-injected by layout.js -->

    <div class="main-container">
      <div class="dashboard-grid">
        <!-- Card 1 -->
        <div class="card">
          <div class="card-header">
            <span>Title</span>
            <span class="status-badge status-online">● Online</span>
          </div>
          <div class="card-body">
            <div class="data-row">
              <div class="data-item">
                <div class="data-label">Label 1</div>
                <div class="data-value">Value 1</div>
              </div>
            </div>
          </div>
        </div>

        <!-- Card 2 -->
        <div class="card">
          <div class="card-header">Title 2</div>
          <div class="card-body">
            <div class="filter-bar">
              <input type="text" class="search-box" placeholder="Search..." />
              <div class="chip">Filter 1</div>
              <div class="chip active">Filter 2</div>
            </div>
            <div class="table-wrapper">
              <table class="table">
                <thead>
                  <tr>
                    <th>Column 1</th>
                    <th>Column 2</th>
                  </tr>
                </thead>
                <tbody>
                  <tr>
                    <td>Data 1</td>
                    <td>Data 2</td>
                  </tr>
                </tbody>
              </table>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Footer auto-injected by layout.js -->

    <script src="assets/layout.js"></script>
    <script src="assets/api-service.js"></script>
    <script src="assets/app.js"></script>
  </body>
</html>
```

---

## 🧪 Testing Checklist

- [ ] layout.js loads without errors (check console)
- [ ] Header title matches page title
- [ ] Active nav link is highlighted
- [ ] Footer updates every 30 seconds
- [ ] Responsive grid shrinks on mobile (768px)
- [ ] Tables have proper hover effects
- [ ] Badges/chips display correctly
- [ ] Status indicators animate smoothly

---

## 📚 Files Reference

| File                        | Purpose                  | Lines |
| --------------------------- | ------------------------ | ----- |
| `assets/layout.js`          | Shared header/nav/footer | 95    |
| `assets/style.css`          | Main stylesheet          | 745   |
| `assets/loading-states.css` | Skeleton animations      | 80+   |
| `assets/api-service.js`     | API client               | 400+  |
| `assets/ui-utils.js`        | UI helpers               | 200+  |
| `assets/app.js`             | Global functions         | 150+  |

---

## 🔄 Migration Notes

### From Individual Header/Nav (Old)

```html
<!-- Each page had its own markup -->
<div class="admin-header">
  <h1>Title</h1>
  <nav class="admin-nav">
    <a href="...">Link</a>
  </nav>
</div>
```

### To Shared Layout (New)

```html
<!-- All pages share layout.js -->
<script src="assets/layout.js"></script>
<!-- Header is auto-generated -->
```

**Benefits**:

- ✅ Single source of truth for navigation
- ✅ Active page highlighting automatic
- ✅ Consistent styling across all pages
- ✅ Easier maintenance & updates
- ✅ Live footer timestamp

---

## 🚀 Future Enhancements

1. **Theme Switching**: Dark mode toggle in footer
2. **Component Library**: Separate file for reusable components
3. **Icon System**: SVG icons for cards/status
4. **Accessibility**: ARIA labels, keyboard navigation
5. **Analytics**: Page view tracking

---

**Last Updated**: 2025-01-15  
**Status**: Production Ready ✅  
**Pages Updated**: 12/12 ✓
